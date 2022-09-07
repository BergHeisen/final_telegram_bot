#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "telegram_client.hpp"
#include "utils.hpp"
#include "yt.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>
#include <unistd.h>

static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

int main(int argc, char **argv) {
  plog::init(plog::debug, &consoleAppender);
  std::string token = get_env_var_or_default(
      "TELEGRAM_TOKEN", "2053605815:AAHpGfk6hHnx1fRktpOmpHl_yCOkZesDOrk");
  TelegramClient client(token.c_str());
  client.start();
  return 0;
}
