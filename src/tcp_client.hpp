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
  void sendData(void *data, const int dataLen);
  void receiveData(void *data, const int dataLenToRead);
  void receiveData(std::stringstream &ss, const int dataLenToRead);
  std::string exchangeMessage(const std::string &message);
  ~TCPConnection();
};
