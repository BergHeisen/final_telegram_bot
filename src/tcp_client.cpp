#include "tcp_client.hpp"
#include "fmt/core.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <plog/Log.h>
#include <sstream>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

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
  // hostent *host_t = gethostbyname(host.c_str());
  sockaddr_in sendSockAddr;
  bzero(&sendSockAddr, sizeof(sendSockAddr));
  // bcopy(host_t->h_addr_list, &sendSockAddr.sin_addr, host_t->h_length);
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_port = htons(port);
  sendSockAddr.sin_addr.s_addr = inet_addr(host.c_str());
  // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEADDR, &enable,
  //            sizeof(int));

  int status = connect(sockfd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));
  if (status < 0) {
    LOGD << fmt::format("Unable to bind to host: {} port: {} errno: {}", host,
                        port, status);
    throw;
  } else {
    LOGD << fmt::format("Successfully bound to host: {} port: {}", host, port);
    connected = true;
  };
}
void TCPConnection::sendData(void *data, const int dataLen) {
  send(sockfd, &dataLen, sizeof(dataLen), 0);
  int ok;
  recv(sockfd, &ok, sizeof(int), 0);
  assert(ok == dataLen);
  send(sockfd, data, dataLen, 0);
}

void TCPConnection::receiveData(void *data, const int dataLenToRead) {
  if (dataLenToRead < 2048) {
    recv(sockfd, data, sizeof(dataLenToRead), 0);
  } else {
  }
}
void TCPConnection::receiveData(std::stringstream &ss,
                                const int dataLenToRead) {
  int bytes_read = 0;
  char buf[2048];
  while (dataLenToRead != bytes_read) {
    memset(&buf, 0, sizeof(buf));
    bytes_read += recv(sockfd, &buf, sizeof(buf), 0);
    ss << buf;
  }
}

std::string TCPConnection::exchangeMessage(const std::string &message) {
  int bytesRead = 0;
  int size = message.size();
  int response_length = 0;
  const char *messageCstring = message.c_str();
  std::stringstream ss;
  LOGD << fmt::format("size = {}", size);
  sendData((void *)messageCstring, size);

  receiveData(&response_length, 4);

  LOGD << fmt::format("Have to read {} bytes", response_length);

  receiveData(ss, response_length);
  LOGD << fmt::format("Server responded with: {}", "success");
  return ss.str();
}

void TCPConnection::closeConnection() {
  try {
    const char *message = "closeConnection";
    sendData((void *)message, strlen(message));
  } catch (...) {
  }
}

TCPConnection::~TCPConnection() {
  closeConnection();
  if (this->connected) {
    close(sockfd);
  }
}

std::shared_ptr<TCPConnection>
TCPConnection::instantiate(const std::string &host, const int port) {

  return std::shared_ptr<TCPConnection>{new TCPConnection(host, port)};
}
