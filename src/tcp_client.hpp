#pragma once
#include <memory>
#include <sstream>
#include <string>

class TCPConnection {

private:
  int sockfd;
  bool connected = false;
  void closeConnection();

  void initiate_connection(const std::string &host, const int port);

public:
  TCPConnection(const std::string &host, const int port);
  static std::shared_ptr<TCPConnection> instantiate(const std::string &host,
                                                    const int port);
  uint32_t receiveDataLength();
  void sendData(void *data, const uint32_t dataLen);
  std::string receiveData(const uint32_t dataLenToRead);
  std::string exchangeMessage(const std::string &message);
  ~TCPConnection();
};
