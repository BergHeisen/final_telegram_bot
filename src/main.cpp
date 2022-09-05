#include "telegram_client.hpp"
#include "utils.hpp"
#include "yt.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
  std::string token = get_env_var_or_default(
      "TELEGRAM_TOKEN", "2053605815:AAHpGfk6hHnx1fRktpOmpHl_yCOkZesDOrk");
  TelegramClient client(token.c_str());
  client.start();
  return 0;
}
