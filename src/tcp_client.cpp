#include "tcp_client.hpp"
#include "fmt/core.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <plog/Log.h>
#include <sstream>
#include <strings.h>
#include <sys/socket.h>

TCPConnection::TCPConnection(const std::string &host, const int port) {
  initiate_connection(host, port);
}

void TCPConnection::initiate_connection(const std::string &host,
                                        const int port) {
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    LOGD << fmt::format("Unable to construct socket, host: {} port: {}", host,
                        port);
    throw;
  }
  const int enable = 1;
  hostent *host_t = gethostbyname(host.c_str());
  sockaddr_in sendSockAddr;
  bzero((char *)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr =
      inet_addr(inet_ntoa(*(struct in_addr *)*host_t->h_addr_list));

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

  int status = connect(sockfd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));
  if (status < 0) {
    LOGD << fmt::format("Unable to bind to host: {} port: {}", host, port);
    throw;
  } else {
    LOGD << fmt::format("Successfully bound to host: {} port: {}", host, port);
    connected = true;
  };
}

std::string TCPConnection::exchangeMessage(const std::string &message) {
  uint16_t bytesRead = 0;
  uint16_t size = message.size();
  uint16_t response_length;
  const char *messageCstring = message.c_str();
  std::stringstream ss;
  send(sockfd, (char *)&size, sizeof(size), 0);
  send(sockfd, (char *)&messageCstring, strlen(messageCstring), 0);
  recv(sockfd, &response_length, sizeof(response_length), 0);
  LOGD << fmt::format("Have to read {} bytes", response_length);
  while (response_length != bytesRead) {
    char buf[1500];
    memset(&buf, 0, sizeof(buf));
    bytesRead += recv(sockfd, &buf, sizeof(buf), 0);
    ss << buf;
    LOGD << fmt::format("Read now {} bytes", bytesRead);
  }
  LOGD << fmt::format("Server responded with: {}", ss.str());
  return ss.str();
}

TCPConnection::~TCPConnection() {
  if (this->connected) {
    close(sockfd);
  }
}

std::shared_ptr<TCPConnection>
TCPConnection::instantiate(const std::string &host, const int port) {

  return std::shared_ptr<TCPConnection>{new TCPConnection(host, port)};
}
