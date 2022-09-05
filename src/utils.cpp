#include "utils.hpp"
#include <array>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <unistd.h>

std::string get_env_var_or_default(std::string const &key,
                                   std::string const defaultValue) {
  char *val = getenv(key.c_str());
  return val == NULL ? defaultValue : std::string(val);
}

std::pair<std::string, int> exec(const char *command,
                                 std::vector<std::string> const &args) {
  std::array<char, 128> buffer;
  std::stringstream commandbuffer;
  commandbuffer << command;
  for (auto &i : args) {
    commandbuffer << " " << i;
  }

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
