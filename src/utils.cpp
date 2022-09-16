#include "utils.hpp"
#include "fmt/core.h"
#include "fmt/format.h"
#include <array>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <cstdlib>
#include <memory>
#include <openssl/sha.h>
#include <plog/Log.h>
#include <sstream>
#include <unistd.h>

std::string get_env_var_or_default(std::string const &key,
                                   std::string const defaultValue) {
  char *val = getenv(key.c_str());
  return val == NULL ? defaultValue : std::string(val);
}

uint64_t getFileSize(const std::string &path) {
  return boost::filesystem::file_size(path);
}

std::string hash(const std::string &p_arg) {
  boost::uuids::detail::sha1 sha1;
  sha1.process_bytes(p_arg.data(), p_arg.size());
  unsigned hash[5] = {0};
  sha1.get_digest(hash);

  // Back to string
  char buf[41] = {0};

  for (int i = 0; i < 5; i++) {
    std::sprintf(buf + (i << 3), "%08x", hash[i]);
  }

  return std::string(buf);
}

std::pair<std::string, int> exec(const char *command,
                                 std::vector<std::string> const &args) {
  std::array<char, 128> buffer;
  std::stringstream commandbuffer;
  commandbuffer << command;
  for (auto &i : args) {
    commandbuffer << " " << i;
  }
  LOGD << fmt::format("Executing: {} with args: {}", command,
                      fmt::join(args.begin(), args.end(), " "));
  std::string cmd = commandbuffer.str();
  std::stringstream result;
  auto file = popen(cmd.c_str(), "r");
  if (!file) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), file) != nullptr) {
    result << buffer.data();
  }
  int returnCode = pclose(file) / 256;
  return std::pair(result.str(), returnCode);
}

std::string fileNameWithoutExtension(const std::string &filepath) {
  return boost::filesystem::path(filepath).stem().string();
}
