#pragma once
#include <string>
#include <vector>

std::pair<std::string, int> exec(const char *command,
                                 std::vector<std::string> const &args);

std::string get_env_var_or_default(std::string const &key,
                                   std::string const defaultValue);

std::string hash(const std::string &input);
uint64_t getFileSize(const std::string &path);
std::string fileNameWithoutExtension(const std::string &filepath);
