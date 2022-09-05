#include "request_handler.hpp"
#include "nlohmann/json.hpp"
#include <random>
#include <type_traits>
namespace {

static RedisClient client;
static std::default_random_engine engine;

template <typename T,
          typename std::enable_if<std::is_base_of<Request, T>::value>::type *>
std::string createRequest(T &request) {
}

template <typename T, typename std::enable_if<
                          std::is_base_of<Request, T>::value>::type * = nullptr>
std::shared_ptr<T> getRequest(std::string &id) {
  client.get(id);
}
