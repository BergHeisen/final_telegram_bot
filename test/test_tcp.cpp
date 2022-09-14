#include "../src/tcp_client.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ostringstream;
using std::string;

string readFileIntoString(const string &path) {
  ifstream input_file = ifstream(path);
  if (!input_file.is_open()) {
    cerr << "Could not open the file - '" << path << "'" << endl;
    exit(EXIT_FAILURE);
  }
  return string((std::istreambuf_iterator<char>(input_file)),
                std::istreambuf_iterator<char>());
}

TEST(TCPConnection, TCPConnection) {
  // auto connection = TCPConnection::instantiate("127.0.0.1", 3436);
  //  auto content = readFileIntoString("./info.json");
}
