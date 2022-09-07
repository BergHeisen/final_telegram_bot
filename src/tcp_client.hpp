#pragma once
#include <memory>
#include <string>

class TCPConnection {

private:
  int sockfd;
  bool connected = false;

  void initiate_connection(const std::string &host, const int port);

public:
  TCPConnection(const std::string &host, const int port);
  static std::shared_ptr<TCPConnection> instantiate(const std::string &host,
                                                    const int port);
  std::string exchangeMessage(const std::string &message);
  ~TCPConnection();
};
