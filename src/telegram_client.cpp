#include "telegram_client.hpp"
#include "fmt/core.h"
#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Logger.h"
#include "plog/Severity.h"
#include "tgbot/Bot.h"
#include "tgbot/TgException.h"
#include "tgbot/net/TgLongPoll.h"
#include <BS_thread_pool.hpp>
#include <iostream>
#include <memory>
#include <plog/Log.h>
#include <unistd.h>

static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

TelegramClient::TelegramClient(const char *token) {
  plog::init(plog::debug, &consoleAppender);
  bot = std::make_unique<Bot>(Bot(token));
  setupMessageHandlers();
}

void handleMessage(TelegramClient *client, Message::Ptr message) {
  PLOGD << "It works";
}

void TelegramClient::setupMessageHandlers() {
  bot->getEvents().onAnyMessage([this](Message::Ptr message) {
    PLOGD << fmt::format("Received message: {} from: {}", message->text,
                         message->from->username);
  });

  bot->getEvents().onNonCommandMessage([this](Message::Ptr message) {
    handleMessage(this, message);
    pool.push_task(&handleMessage, this, message);
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
