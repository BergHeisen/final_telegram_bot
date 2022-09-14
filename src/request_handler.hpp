#pragma once
#include "utils.hpp"
#include <BS_thread_pool.hpp>
#include <climits>
#include <cpp_redis/core/client.hpp>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <type_traits>

static std::random_device engine;
const int REQUEST_VERSION = 1;

class RedisClient {
private:
  std::unique_ptr<cpp_redis::client> client =
      std::make_unique<cpp_redis::client>();
  RedisClient();

public:
  void connect(std::string &host, int port) const;
  void set(std::string &key, std::string &value) const;
  std::string get(std::string &key) const;
  // static std::shared_ptr<RedisClient> getInstance() {
  static RedisClient &getInstance() {
    // static std::shared_ptr<RedisClient> s{new RedisClient};
    static RedisClient s;
    return s;
  };
};

struct GenericRequest {
  int requestVersion = 1;
  std::string operation = "download";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GenericRequest, requestVersion, operation)

struct DownloadRequest {
  std::string fileIdentifier = "";
  bool audioOnly = "false";
  std::string resolution = "";
  int requestVersion = 1;
  std::string operation = "download";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DownloadRequest, requestVersion, operation,
                                   fileIdentifier, audioOnly, resolution);

template <typename T>

std::string createRequest(T &request) {
  RedisClient &client = RedisClient::getInstance();
  std::string id = std::to_string(
      std::uniform_int_distribution<long>(LONG_MIN, LONG_MAX)(engine));
  nlohmann::json r = nlohmann::json(request);
  std::string requestJson = r.dump();
  client.set(id, requestJson);
  return id;
}

template <typename T> std::shared_ptr<T> getRequest(std::string &id) {
  const RedisClient &client = RedisClient::getInstance();
  const std::string jsonString = client.get(id);
  const nlohmann::json j = nlohmann::json::parse(jsonString);
  const auto request = j.get<T>();
  return std::make_shared<T>(request);
}
