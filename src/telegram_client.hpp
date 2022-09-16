#pragma once
#include "BS_thread_pool.hpp"
#include <memory>
#include <tgbot/tgbot.h>
using namespace TgBot;

class TelegramClient {
private:
  std::unique_ptr<Bot> bot;
  BS::thread_pool pool;

  void setupMessageHandlers();

public:
  const char *token;
  TelegramClient(const char *token);
  void start();
};
