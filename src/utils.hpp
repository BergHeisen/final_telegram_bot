#pragma once
#include <string>
#include <vector>

std::pair<std::string, int> exec(const char *command,
                                 std::vector<std::string> const &args);

std::string get_env_var_or_default(std::string const &key,
                                   std::string const defaultValue);