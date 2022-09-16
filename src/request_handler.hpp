#pragma once
#include "fmt/core.h"
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
  void connect(const std::string &host, const int port) const;
  void set(const std::string &key, const std::string &value) const;
  std::string get(const std::string &key) const;
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

  const std::string id() const {
    return hash(fmt::format("{}{}{}{}{}", fileIdentifier, audioOnly, resolution,
                            requestVersion, operation));
  }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DownloadRequest, requestVersion, operation,

                                   fileIdentifier, audioOnly, resolution);

struct ConvertProvidedRequest {
  std::string fileIdentifier = "";
  std::string target = "";
  int requestVersion = 1;
  std::string operation = "convertProvided";

  const std::string id() const {
    return hash(fmt::format("{}{}{}{}", fileIdentifier, target, requestVersion,
                            operation));
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConvertProvidedRequest, requestVersion,
                                   operation, fileIdentifier, target);

struct ConvertURLRequest {
  std::string fileUrl = "";
  std::string target = "";
  int requestVersion = 1;
  std::string operation = "convertURL";

  const std::string id() const {
    return hash(
        fmt::format("{}{}{}{}", fileUrl, target, requestVersion, operation));
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConvertURLRequest, requestVersion, operation,
                                   fileUrl, target);

template <typename T>

std::string createRequest(T &request) {
  RedisClient &client = RedisClient::getInstance();
  // std::string id = std::to_string(
  //     std::uniform_int_distribution<long>(LONG_MIN, LONG_MAX)(engine));
  // std::string id = hash(std::string(reinterpret_cast<char *>(&request)));
  std::string id = request.id();
  nlohmann::json r = nlohmann::json(request);
  std::string requestJson = r.dump();
  client.set(id, requestJson);
  return id;
}

template <typename T> std::shared_ptr<T> getRequest(const std::string &id) {
  const RedisClient &client = RedisClient::getInstance();
  const std::string jsonString = client.get(id);
  const nlohmann::json j = nlohmann::json::parse(jsonString);
  const auto request = j.get<T>();
  return std::make_shared<T>(request);
}
