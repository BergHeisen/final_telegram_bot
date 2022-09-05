#pragma once
#include <cpp_redis/core/client.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
class RedisClient {
private:
  std::unique_ptr<cpp_redis::client> client =
      std::make_unique<cpp_redis::client>();

public:
  RedisClient() { client->connect(); }
  void set(std::string &key, std::string &value) { client->set(key, value); }
  std::string get(std::string &key) {
    auto reply = client->get(key).get();
    if (reply.is_null() || reply.is_error()) {
      return "";
    } else {
      return reply.as_string();
    }
  }
};

struct Request {
  std::string requestVersion;
  std::string operation;
  std::string fileIdentifier;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Request, requestVersion, operation,
                                   fileIdentifier);
struct DownloadRequest : public Request {};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DownloadRequest, requestVersion, operation,
                                   fileIdentifier);

template <typename T,
          typename std::enable_if<std::is_base_of<Request, T>::value>::type *>

std::string createRequest(T &request) {
  std::string id = std::to_string(
      std::uniform_int_distribution<long>(LONG_MIN, LONG_MAX)(engine));
  // nlohmann::json r;
  // r["requestVersion"] = request.requestVersion;
  // r["operation"] = request.operation;
  // r["fileIdentifier"] = request.fileIdentifier;
  auto r = to_json(request);
  std::string requestJson = r.dump();
  client.set(id, requestJson);
  return id;
}

template <typename T,
          typename std::enable_if<std::is_base_of<Request, T>::value>::type *>
std::shared_ptr<T> getRequest(const char *id) {}
