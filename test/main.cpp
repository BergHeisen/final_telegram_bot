#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include <gtest/gtest.h>
#include <iostream>
static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
int main(int argc, char **argv) {
  plog::init(plog::debug, &consoleAppender);
  ::testing::InitGoogleTest(&argc, argv);

  std::cout << "RUNNING TESTS ... " << std::endl;
  int ret{RUN_ALL_TESTS()};
  if (!ret) {
    std::cout << "SUCCESS" << std::endl;
  } else
    std::cout << "FAILURE" << std::endl;
  return 0;
}
