#include "telegram_client.hpp"
#include "file_provider.hpp"
#include "fmt/core.h"
#include "mime_types.hpp"
#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Logger.h"
#include "plog/Severity.h"
#include "request_handler.hpp"
#include "tgbot/Bot.h"
#include "tgbot/TgException.h"
#include "tgbot/net/TgLongPoll.h"
#include "tgbot/types/CallbackQuery.h"
#include "tgbot/types/GenericReply.h"
#include "tgbot/types/InlineKeyboardButton.h"
#include "tgbot/types/InlineKeyboardMarkup.h"
#include "yt.hpp"
#include <BS_thread_pool.hpp>
#include <algorithm>
#include <iostream>
#include <memory>
#include <plog/Log.h>
#include <string>
#include <unistd.h>

std::string sanitizeForMarkdownV2(std::string);
static MimeTypes mime_types = MimeTypes();

TelegramClient::TelegramClient(const char *token) {
  bot = std::make_unique<Bot>(Bot(token));
  setupMessageHandlers();
}

std::shared_ptr<GenericReply> generateKeyboard(const VideoInformation &info) {
  auto resolutions = getAvailableResolutions(info);
  auto keyboard = std::make_shared<InlineKeyboardMarkup>();
  int currentIndex = 0;

  keyboard->inlineKeyboard =
      std::vector<std::vector<InlineKeyboardButton::Ptr>>();
  if (info.extractor == "youtube") {
    currentIndex++;
    auto button = std::vector<InlineKeyboardButton::Ptr>(
        {std::make_shared<InlineKeyboardButton>()});
    DownloadRequest requestInfo{info.url, true};
    std::string request = createRequest<DownloadRequest>(requestInfo);
    LOGD << fmt::format("Adding Request for AudioDownload of url: {} to {}",
                        info.title, request);
    button[0]->text = "Audio Only";
    button[0]->callbackData = request;
    keyboard->inlineKeyboard.push_back(button);
  }
  if (resolutions.empty()) {
    DownloadRequest requestInfo{info.url, false};
    std::string request = createRequest(requestInfo);
    auto button = std::vector<InlineKeyboardButton::Ptr>(
        {std::make_shared<InlineKeyboardButton>()});
    button[0]->text = "Download Video";
    button[0]->callbackData = request;
    keyboard->inlineKeyboard.push_back(button);
    return keyboard;
  }

  for (auto &res : resolutions) {
    if (keyboard->inlineKeyboard.size() == currentIndex) {
      keyboard->inlineKeyboard.push_back(
          std::vector<InlineKeyboardButton::Ptr>());
    }
    DownloadRequest requestInfo{info.url, false, std::to_string(res)};
    std::string request = createRequest(requestInfo);
    auto button = std::make_shared<InlineKeyboardButton>();
    button->text = std::to_string(res) + "p";
    button->callbackData = request;
    keyboard->inlineKeyboard[currentIndex].push_back(button);
    if (keyboard->inlineKeyboard[currentIndex].size() == 2)
      currentIndex++;
  }
  return keyboard;
};

bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos +=
        to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return true;
}

void handleMessage(Bot *client, Message::Ptr message) {
  auto [videoInfo, exitCode] = getVideoInformationJSON(message->text);
  if (exitCode.success == true) {
    auto keyboard = generateKeyboard(*videoInfo.get());
    std::string reply = fmt::format("Requesting to download:\n\n*{}*\nfrom: "
                                    "*{}*\n\n_Choose on the options below_",
                                    sanitizeForMarkdownV2(videoInfo->title),
                                    sanitizeForMarkdownV2(videoInfo->uploader));
    if (!videoInfo->thumbnails.has_value()) {
      client->getApi().sendMessage(message->chat->id, reply, false,
                                   message->messageId, keyboard, "MarkdownV2");
    } else {
      std::string thumbnailUrl = getThumbnail(*videoInfo.get());
      client->getApi().sendPhoto(message->chat->id, thumbnailUrl, reply,
                                 message->messageId, keyboard, "MarkdownV2");
    }
  } else {
    client->getApi().sendMessage(message->chat->id,
                                 "Invalid URL has been given!");
  }
}

void handleQuery(Bot *bot, CallbackQuery::Ptr callbackQuery) {
  auto processingMessage = bot->getApi().sendMessage(
      callbackQuery->message->chat->id,
      fmt::format("*{}*", sanitizeForMarkdownV2("Processing Request")), false,
      callbackQuery->message->messageId, std::make_shared<GenericReply>(),
      "MarkdownV2");
  const auto genericRequest = getRequest<GenericRequest>(callbackQuery->data);
  PLOGD << fmt::format("Request id: {}, version: {} operation: {}",
                       callbackQuery->data, genericRequest->requestVersion,
                       genericRequest->operation);

  if (genericRequest->operation == "download") {
    const auto downloadRequest =
        getRequest<DownloadRequest>(callbackQuery->data);
    const auto response =
        download_video(downloadRequest->fileIdentifier,
                       downloadRequest->resolution, downloadRequest->audioOnly);
    if (response.success) {
      insertFile(response.file_path, downloadRequest->fileIdentifier,
                 response.title);
      bot->getApi().editMessageText(
          fmt::format("*{}*", sanitizeForMarkdownV2("Uploading File...")),
          processingMessage->chat->id, processingMessage->messageId, "",
          "MarkDownV2");
      if (downloadRequest->audioOnly) {
        bot->getApi().sendAudio(
            callbackQuery->message->chat->id,
            InputFile::fromFile(
                response.file_path,
                mime_types.getType(response.file_path.c_str())));
      } else {
        bot->getApi().sendVideo(
            callbackQuery->message->chat->id,
            InputFile::fromFile(
                response.file_path,
                mime_types.getType(response.file_path.c_str())));
      }
    } else {
      bot->getApi().sendMessage(callbackQuery->message->chat->id,
                                fmt::format("*{}*", response.error_msg), false,
                                callbackQuery->message->messageId, nullptr,
                                "MarkDownV2");
      PLOGE << fmt::format("Error occurred during download of file identifier: "
                           "{}, error_msg: {}",
                           callbackQuery->data, response.error_msg);
    }
  }
  bot->getApi().deleteMessage(processingMessage->chat->id,
                              processingMessage->messageId);
}

void TelegramClient::setupMessageHandlers() {
  bot->getEvents().onAnyMessage([](Message::Ptr message) {
    PLOGD << fmt::format("Received message: {} from: {}", message->text,
                         message->from->username);
  });

  bot->getEvents().onNonCommandMessage([this](Message::Ptr message) {
    pool.push_task(&handleMessage, bot.get(), message);
  });

  bot->getEvents().onCallbackQuery([this](CallbackQuery::Ptr callbackQuery) {
    pool.push_task(&handleQuery, bot.get(), callbackQuery);
  });
}

void TelegramClient::start() {
  auto poll = TgLongPoll(*bot.get());
  try {
    PLOGI << fmt::format("Logged in with token as Bot: {}",
                         bot->getApi().getMe()->username);
    while (true) {
      PLOGI << fmt::format("Starting long poll");
      poll.start();
    }
  } catch (TgException &e) {
    PLOGE << "Error Occured!";
    PLOGE << e.what();
  }
}

std::string sanitizeForMarkdownV2(std::string str) {
  replace(str, "_", "\\_");
  replace(str, "*", "\\*");
  replace(str, "[", "\\[");
  replace(str, "]", "\\]");
  replace(str, "(", "\\(");
  replace(str, ")", "\\)");
  replace(str, "~", "\\~");
  replace(str, "`", "\\`");
  replace(str, ">", "\\>");
  replace(str, "#", "\\#");
  replace(str, "+", "\\+");
  replace(str, "-", "\\-");
  replace(str, "=", "\\=");
  replace(str, "|", "\\|");
  replace(str, "{", "\\{");
  replace(str, "}", "\\}");
  replace(str, "!", "\\!");
  replace(str, ".", "\\.");
  std::cout << str << std::endl;
  return str;
}
