#include "tcp_client.hpp"
#include "fmt/core.h"
#include <_types/_uint32_t.h>
#include <arpa/inet.h>
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
void TCPConnection::sendData(void *data, const uint32_t dataLen) {
  const uint32_t dataLength = htonl(dataLen);
  send(sockfd, &dataLength, sizeof(uint32_t), 0);
  send(sockfd, data, dataLen, 0);
}

std::string TCPConnection::receiveData(const uint32_t dataLenToRead) {
  uint32_t bytes_read = 0;
  std::vector<uint8_t> buf;
  buf.resize(dataLenToRead, 0x00);
  while (bytes_read < dataLenToRead) {
    bytes_read +=
        recv(sockfd, &(buf[bytes_read]), dataLenToRead - bytes_read, 0);
  }
  LOGD << fmt::format("Bytes read: {}", bytes_read);
  std::string result;
  return result.assign(buf.begin(), buf.end());
}

uint32_t TCPConnection::receiveDataLength() {
  uint32_t dataLength;
  recv(sockfd, &dataLength, sizeof(uint32_t), 0);
  return ntohl(dataLength);
}

std::string TCPConnection::exchangeMessage(const std::string &message) {
  int bytesRead = 0;
  uint32_t size = message.size();
  int response_length = 0;
  const char *messageCstring = message.c_str();
  std::stringstream ss;
  LOGD << fmt::format("size = {}", size);
  sendData((void *)messageCstring, size);
  uint32_t dataLength = receiveDataLength();
  LOGD << fmt::format("Have to read {} bytes", dataLength);
  std::string output = receiveData(dataLength);
  LOGD << fmt::format("Server responded with: {}", "success");
  return output;
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
