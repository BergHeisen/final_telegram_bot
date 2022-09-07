#include "../src/tcp_client.hpp"
#include <gtest/gtest.h>
#include <string>

TEST(TCPConnection, TCPConnection) {
  auto connection = TCPConnection::instantiate("127.0.0.1", 3434);
}
