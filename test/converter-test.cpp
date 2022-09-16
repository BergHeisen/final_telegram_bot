#include "../src/converter.hpp"
#include "gtest/gtest.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

TEST(ConverterSuite, ConvertMP4) {
  Converter *converter = getConverter("mp4");
  converter->convert("./testvideos/webm_video.webm",
                     "./testvideos/webm_video.mp4");
  ASSERT_EQ(boost::filesystem::exists("./testvideos/webm_video.mp4"), true);
}
