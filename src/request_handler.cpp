#include "request_handler.hpp"
#include "cpp_redis/core/types.hpp"
#include "fmt/core.h"
#include "nlohmann/json.hpp"
#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Logger.h"
#include <memory>
#include <plog/Log.h>
#include <unistd.h>

RedisClient::RedisClient() {
  std::string host = get_env_var_or_default("REDIS_HOST", "127.0.0.1");
  std::string port = get_env_var_or_default("REDIS_PORT", "6379");
  connect(host, std::stoi(port));
  client->sync_commit();
}
void RedisClient::set(std::string &key, std::string &value) const {
  auto future = client->set(key, value);
  client->sync_commit();
  const auto reply = future.get();
  if (reply.is_error()) {
    LOGE << fmt::format("Error occured with key: {} value: {}, error: {} ", key,
                        value, reply.error());
  } else {
    LOGD << fmt::format("Setting key: {} value: {}, ok: {}", key, value,
                        reply.ok());
  }
  return;
}

std::string RedisClient::get(std::string &key) const {
  auto future = client->get(key);
  client->sync_commit();
  const auto reply = future.get();
  if (reply.is_null() || reply.is_error()) {
    return "";
  } else {
    return reply.as_string();
  }
}

void RedisClient::connect(std::string &host, int port) const {
  const cpp_redis::connect_callback_t idk;
  client->connect(host, port,
                  [](const std::string &host, std::size_t port,
                     cpp_redis::connect_state status) {
                    LOGD << fmt::format("Connect State on {}:{}", host, port);
                  });
}
