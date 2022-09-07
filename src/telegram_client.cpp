#include "telegram_client.hpp"
#include "fmt/core.h"
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
#include <iostream>
#include <memory>
#include <plog/Log.h>
#include <string>
#include <unistd.h>

static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
std::string sanitizeForMarkdownV2(std::string);

TelegramClient::TelegramClient(const char *token) {
  plog::init(plog::debug, &consoleAppender);
  bot = std::make_unique<Bot>(Bot(token));
  setupMessageHandlers();
}

std::shared_ptr<GenericReply> generateKeyboard(VideoInformation &info) {
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
    button[0]->callbackData = "Nothing";
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
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

void handleMessage(Bot *client, Message::Ptr message) {
  auto [videoInfo, exitCode] = getVideoInformationJSON(message->text.c_str());
  if (exitCode == 0) {
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

void TelegramClient::setupMessageHandlers() {
  bot->getEvents().onAnyMessage([](Message::Ptr message) {
    PLOGD << fmt::format("Received message: {} from: {}", message->text,
                         message->from->username);
  });

  bot->getEvents().onNonCommandMessage([this](Message::Ptr message) {
    pool.push_task(&handleMessage, bot.get(), message);
  });

  bot->getEvents().onCallbackQuery([this](CallbackQuery::Ptr callbackQuery) {
    bot->getApi().sendMessage(
        callbackQuery->message->chat->id,
        fmt::format("*{}*", sanitizeForMarkdownV2("Processing Request")), false,
        callbackQuery->message->messageId, std::make_shared<GenericReply>(),
        "MarkdownV2");
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
  return str;
}
