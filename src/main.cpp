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
  auto [direct, directCode] = getVideoInformationJSON(
      "https://file-examples.com/storage/fe7d3a0d44631509da1f416/2020/03/"
      "file_example_WEBM_640_1_4MB.webm");
  auto [twitter, twitterCode] = getVideoInformationJSON(
      "https://twitter.com/lmfaoos/status/"
      "1566050431956140038?s=21&t=pHCkCGt3I0CxSE0K1Jht8Q");
  // client.start();
  auto [info, code] = getVideoInformationJSON(
      "https://www.reddit.com/r/dankvideos/comments/x5fq5o/"
      "lifedeath_artist_matt_furie/");
  auto [youtubeInfo, youtubeCode] =
      getVideoInformationJSON("https://www.youtube.com/watch?v=Zi_XLOBDo_Y");
  auto idk = getAvailableResolutions(*direct.get());
  for (auto &i : idk) {
    std::cout << i << std::endl;
  }
  return 0;
}
