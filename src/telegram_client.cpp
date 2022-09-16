#include "telegram_client.hpp"
#include "converter.hpp"
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
#include "tgbot/types/Message.h"
#include "utils.hpp"
#include "yt.hpp"
#include <BS_thread_pool.hpp>
#include <algorithm>
#include <boost/filesystem/convenience.hpp>
#include <iostream>
#include <memory>
#include <plog/Log.h>
#include <string>
#include <unistd.h>
#include <vector>
#define TELEGRAM_UPLOAD_LIMIT 50 * 1000 * 1000

const std::string host = get_env_var_or_default(
    "SERVER_URL",
    fmt::format("127.0.0.1:{}",
                get_env_var_or_default("PORT", std::to_string(4040))));
std::string sanitizeForMarkdownV2(std::string);
static MimeTypes mime_types = MimeTypes();

std::string getFileURLFromTelegram(const std::string &token,
                                   const std::string &fileId) {
  return fmt::format("https://api.telegram.org/file/bot{}/{}", token, fileId);
}

TelegramClient::TelegramClient(const char *token) : token(token) {
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

std::shared_ptr<GenericReply>
generateKeyboardForConvert(const std::string &mimeType,
                           const std::string &fileURL,
                           const std::string &token) {
  auto keyboard = std::make_shared<InlineKeyboardMarkup>();

  keyboard->inlineKeyboard =
      std::vector<std::vector<InlineKeyboardButton::Ptr>>();
  {
    auto button = std::vector<InlineKeyboardButton::Ptr>(
        {std::make_shared<InlineKeyboardButton>()});
    auto requestData = ConvertURLRequest{fileURL, "gif"};
    auto request = createRequest(requestData);
    button[0]->text = "Convert to GIF";
    button[0]->callbackData = request;
    keyboard->inlineKeyboard.push_back(button);
  }

  {
    auto secondRow = std::vector<InlineKeyboardButton::Ptr>();
    if (mimeType.find("mp4") != std::string::npos) {
      auto requestData = ConvertURLRequest{fileURL, "webm"};
      const auto request = createRequest(requestData);
      const auto button = std::make_shared<InlineKeyboardButton>();
      button->text = "Convert to WEBM";
      secondRow.push_back(button);
      button->callbackData = request;
    } else if (mimeType.find("webm") != std::string::npos) {
      auto requestData = ConvertURLRequest{fileURL, "mp4"};
      const auto request = createRequest(requestData);
      const auto button = std::make_shared<InlineKeyboardButton>();
      button->text = "Convert to MP4";
      button->callbackData = request;
      secondRow.push_back(button);
    } else {
      auto requestDataMP4 = ConvertURLRequest{fileURL, "mp4"};
      const auto requestMP4 = createRequest(requestDataMP4);
      const auto buttonMP4 = std::make_shared<InlineKeyboardButton>();
      auto requestDataWEBM = ConvertURLRequest{fileURL, "webm"};
      const auto requestWEBM = createRequest(requestDataWEBM);
      const auto buttonWEBM = std::make_shared<InlineKeyboardButton>();
      buttonMP4->text = "Convert to MP4";
      buttonMP4->callbackData = requestMP4;

      buttonWEBM->text = "Convert to WEBM";
      buttonWEBM->callbackData = requestWEBM;

      secondRow.push_back(buttonMP4);
      secondRow.push_back(buttonWEBM);
    }
    auto requestData = ConvertURLRequest{fileURL, "mp3"};
    const auto request = createRequest(requestData);
    const auto button = std::make_shared<InlineKeyboardButton>();
    button->text = "Convert to MP3";
    button->callbackData = request;
    secondRow.push_back(button);
    keyboard->inlineKeyboard.push_back(secondRow);
  }
  return keyboard;
}
void handleMessage(TelegramClient *tgclient, Bot *client,
                   Message::Ptr message) {
  if (message->video.get() == nullptr && message->document.get() == nullptr) {
    auto [videoInfo, exitCode] = getVideoInformationJSON(message->text);
    if (exitCode.success == true) {
      auto keyboard = generateKeyboard(*videoInfo.get());
      std::string reply =
          fmt::format("Requesting to download:\n\n*{}*\nfrom: "
                      "*{}*\n\n_Choose one of the options below_",
                      sanitizeForMarkdownV2(videoInfo->title),
                      sanitizeForMarkdownV2(videoInfo->uploader));
      if (!videoInfo->thumbnails.has_value()) {
        client->getApi().sendMessage(message->chat->id, reply, false,
                                     message->messageId, keyboard,
                                     "MarkdownV2");
      } else {
        std::string thumbnailUrl = getThumbnail(*videoInfo.get());
        try {
          client->getApi().sendPhoto(message->chat->id, thumbnailUrl, reply,
                                     message->messageId, keyboard,
                                     "MarkdownV2");
        } catch (TgException &e) {
          client->getApi().sendMessage(message->chat->id, reply, false,
                                       message->messageId, keyboard,
                                       "MarkdownV2");
        }
      }
    } else {
      client->getApi().sendMessage(message->chat->id,
                                   "Invalid URL has been given!");
    }
  } else {
    const std::string reply =
        fmt::format("*Requested to Convert Quoted Video*\n\n_{}_",
                    sanitizeForMarkdownV2("Choose one of the options Below"));
    if (message->video.get() != nullptr) {
      const std::string fileUrl = getFileURLFromTelegram(
          tgclient->token,
          client->getApi().getFile(message->video->fileId)->filePath);
      const auto keyboard = generateKeyboardForConvert(
          message->video->mimeType, fileUrl, tgclient->token);
      client->getApi().sendMessage(message->chat->id, reply, false,
                                   message->messageId, keyboard, "MarkdownV2");
    } else if (message->document.get() != nullptr) {
      if (message->document->mimeType.find("webm") != std::string::npos) {
        const std::string fileUrl = getFileURLFromTelegram(
            tgclient->token,
            client->getApi().getFile(message->document->fileId)->filePath);
        const auto keyboard = generateKeyboardForConvert(
            message->document->mimeType, fileUrl, tgclient->token);

        client->getApi().sendMessage(message->chat->id, reply, false,
                                     message->messageId, keyboard,
                                     "MarkdownV2");
      }
    }
  }
}

std::shared_ptr<GenericReply>
generateVideoReplyKeyboard(const std::string &id, const std::string filePath) {
  auto keyboard = std::make_shared<InlineKeyboardMarkup>();
  const bool hasAudio = fileHasAudio(filePath);
  const std::string extension =
      boost::filesystem::extension(filePath).substr(1);
  PLOGD << extension;
  keyboard->inlineKeyboard =
      std::vector<std::vector<InlineKeyboardButton::Ptr>>();
  {
    auto button = std::vector<InlineKeyboardButton::Ptr>(
        {std::make_shared<InlineKeyboardButton>()});
    auto requestData = ConvertProvidedRequest{id, "gif"};
    const auto request = createRequest(requestData);
    button[0]->text = "Convert to GIF";
    button[0]->callbackData = request;
    keyboard->inlineKeyboard.push_back(button);
  }

  {
    auto secondRow = std::vector<InlineKeyboardButton::Ptr>({});
    if (extension == "mp4") {
      auto requestData = ConvertProvidedRequest{id, "webm"};
      const auto request = createRequest(requestData);
      const auto button = std::make_shared<InlineKeyboardButton>();
      button->text = "Convert to WEBM";
      secondRow.push_back(button);
      button->callbackData = request;
    } else if (extension == "webm") {
      auto requestData = ConvertProvidedRequest{id, "mp4"};
      const auto request = createRequest(requestData);
      const auto button = std::make_shared<InlineKeyboardButton>();
      button->text = "Convert to MP4";
      button->callbackData = request;
      secondRow.push_back(button);
    } else {
      auto requestDataMP4 = ConvertProvidedRequest{id, "mp4"};
      const auto requestMP4 = createRequest(requestDataMP4);
      const auto buttonMP4 = std::make_shared<InlineKeyboardButton>();
      auto requestDataWEBM = ConvertProvidedRequest{id, "webm"};
      const auto requestWEBM = createRequest(requestDataWEBM);
      const auto buttonWEBM = std::make_shared<InlineKeyboardButton>();
      buttonMP4->text = "Convert to MP4";
      buttonMP4->callbackData = requestMP4;

      buttonWEBM->text = "Convert to WEBM";
      buttonWEBM->callbackData = requestWEBM;

      secondRow.push_back(buttonMP4);
      secondRow.push_back(buttonWEBM);
    }
    if (hasAudio) {
      auto requestData = ConvertProvidedRequest{id, "mp3"};
      const auto request = createRequest(requestData);
      const auto button = std::make_shared<InlineKeyboardButton>();
      button->text = "Convert to MP3";
      button->callbackData = request;
      secondRow.push_back(button);
    }
    keyboard->inlineKeyboard.push_back(secondRow);
    auto thirdRow = std::vector<InlineKeyboardButton::Ptr>({});
    const auto button = std::make_shared<InlineKeyboardButton>();
    button->text = "Direct Link To Video";
    button->url = fmt::format("http://{}/video/{}", host, id);
    thirdRow.push_back(button);
    keyboard->inlineKeyboard.push_back(thirdRow);
  }

  return keyboard;
}

void handleDownloadRequest(Bot *bot, CallbackQuery::Ptr callbackQuery,
                           Message::Ptr processingMessage) {
  const auto downloadRequest = getRequest<DownloadRequest>(callbackQuery->data);
  const auto response =
      download_video(downloadRequest->fileIdentifier,
                     downloadRequest->resolution, downloadRequest->audioOnly);
  if (response.success) {
    const std::string id = insertFile(
        response.file_path, downloadRequest->fileIdentifier, response.title);
    bot->getApi().editMessageText(
        fmt::format("*{}*", sanitizeForMarkdownV2("Uploading File...")),
        processingMessage->chat->id, processingMessage->messageId, "",
        "MarkDownV2");
    if (downloadRequest->audioOnly) {
      bot->getApi().sendAudio(
          callbackQuery->message->chat->id,
          InputFile::fromFile(response.file_path,
                              mime_types.getType(response.file_path.c_str())));
    } else {
      const auto replyMarkup =
          generateVideoReplyKeyboard(id, response.file_path);
      if (getFileSize(response.file_path) < TELEGRAM_UPLOAD_LIMIT) {
        bot->getApi().sendVideo(
            callbackQuery->message->chat->id,
            InputFile::fromFile(response.file_path,
                                mime_types.getType(response.file_path.c_str())),
            false, 0, 0, 0, "", "", 0, replyMarkup);
      } else {
        bot->getApi().sendMessage(
            callbackQuery->message->chat->id,
            fmt::format("*{}*", sanitizeForMarkdownV2(
                                    "Large is too file to upload on Telegram, "
                                    "use the direct link!")),
            false, 0, replyMarkup, "MarkDownV2");
      }
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
void handleConvertRequest(Bot *bot, std::string fileIdentifier,
                          Converter *converter,
                          Message::Ptr processingMessage) {
  const auto file = getFile(fileIdentifier);
  const auto target = converter->getExtension();
  if (file.has_value()) {
    const std::string convertedFilename =
        fmt::format("{}.{}", file->title, target);
    const std::string out = fmt::format("{}/{}", file->path, convertedFilename);
    auto providedFile = getFile(file->urlHash, convertedFilename);

    if (!providedFile.has_value()) {
      const int statusCode = converter->convert(file->filePath, out);
      providedFile = getFile(insertFile(out, file->urlHash, "", false));
      if (statusCode != 0) {
        bot->getApi().sendMessage(
            processingMessage->chat->id,
            fmt::format("*{}*", sanitizeForMarkdownV2(
                                    "Error occurred during convertion!")),
            false, 0, nullptr, "MarkdownV2");
        return;
      }
    }

    const auto keyboard = std::make_shared<InlineKeyboardMarkup>();
    keyboard->inlineKeyboard =
        std::vector<std::vector<InlineKeyboardButton::Ptr>>();
    keyboard->inlineKeyboard.push_back(std::vector<InlineKeyboardButton::Ptr>(
        {std::make_shared<InlineKeyboardButton>()}));
    keyboard->inlineKeyboard[0][0]->text = "Direct Link To File";
    keyboard->inlineKeyboard[0][0]->url =
        fmt::format("http://{}/video/{}", host, providedFile->id);

    // bot->getApi().editMessageText(
    //     fmt::format("*{}*", sanitizeForMarkdownV2("Uploading File...")),
    //     processingMessage->chat->id, processingMessage->messageId, "",
    //     "MarkDownV2");
    // if (target == "mp4" || target == "webm") {
    //   bot->getApi().sendVideo(
    //       processingMessage->chat->id,
    //       InputFile::fromFile(
    //           out, mime_types.getType(providedFile->filePath.c_str())));
    // } else if (target == "mp3") {
    //   bot->getApi().sendAudio(
    //       processingMessage->chat->id,
    //       InputFile::fromFile(
    //           out, mime_types.getType(providedFile->filePath.c_str())));
    // } else if (target == "gif") {
    //   bot->getApi().sendAnimation(
    //       processingMessage->chat->id,
    //       InputFile::fromFile(
    //           out, mime_types.getType(providedFile->filePath.c_str())));
    // }

    bot->getApi().editMessageText(
        fmt::format("*{}*", sanitizeForMarkdownV2("Uploading File...")),
        processingMessage->chat->id, processingMessage->messageId, "",
        "MarkDownV2");

    if (getFileSize(providedFile->filePath) > TELEGRAM_UPLOAD_LIMIT) {
      bot->getApi().sendMessage(
          processingMessage->chat->id,
          fmt::format("*{}*", sanitizeForMarkdownV2(
                                  "Large is too file to upload on Telegram, "
                                  "use the direct link!")),
          false, 0, keyboard, "MarkDownV2");
      return;
    }

    if (target == "mp4" || target == "webm") {
      bot->getApi().sendVideo(
          processingMessage->chat->id,
          InputFile::fromFile(
              providedFile->filePath,
              mime_types.getType(providedFile->filePath.c_str())),
          false, 0, 0, 0, "", "", 0, keyboard);
    } else if (target == "mp3") {
      bot->getApi().sendAudio(
          processingMessage->chat->id,
          InputFile::fromFile(
              providedFile->filePath,
              mime_types.getType(providedFile->filePath.c_str())));
    } else if (target == "gif") {
      bot->getApi().sendAnimation(
          processingMessage->chat->id,
          InputFile::fromFile(
              providedFile->filePath,
              mime_types.getType(providedFile->filePath.c_str())));
    }
  }
}

void handleConvertURLRequest(Bot *bot, CallbackQuery::Ptr callbackQuery,
                             Message::Ptr processingMessage) {
  const auto convertReqeust =
      getRequest<ConvertURLRequest>(callbackQuery->data);
  const auto converter = getConverter(convertReqeust->target);
  const auto downloadResponse =
      download_video(convertReqeust->fileUrl, "", false);
  if (downloadResponse.success) {
    const std::string identifier =
        insertFile(downloadResponse.file_path, convertReqeust->fileUrl,
                   downloadResponse.title);
    handleConvertRequest(bot, identifier, converter, processingMessage);
  } else {
    bot->getApi().sendMessage(callbackQuery->message->chat->id,
                              fmt::format("*{}*", downloadResponse.error_msg),
                              false, callbackQuery->message->messageId, nullptr,
                              "MarkDownV2");
    PLOGE << fmt::format("Error occurred during download of file url: "
                         "{}, error_msg: {}",
                         convertReqeust->fileUrl, downloadResponse.error_msg);
  }
}

void handleConvertProvidedRequest(Bot *bot, CallbackQuery::Ptr callbackQuery,
                                  Message::Ptr processingMessage) {
  const auto convertReqeust =
      getRequest<ConvertProvidedRequest>(callbackQuery->data);
  const auto converter = getConverter(convertReqeust->target);
  handleConvertRequest(bot, convertReqeust->fileIdentifier, converter,
                       processingMessage);
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
    handleDownloadRequest(bot, callbackQuery, processingMessage);
  } else if (genericRequest->operation == "convertProvided") {
    handleConvertProvidedRequest(bot, callbackQuery, processingMessage);
  } else if (genericRequest->operation == "convertURL") {
    handleConvertURLRequest(bot, callbackQuery, processingMessage);
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
    pool.push_task(&handleMessage, this, bot.get(), message);
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
  return str;
}
