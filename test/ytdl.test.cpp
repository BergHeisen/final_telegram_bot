#include "../src/yt.hpp"
#include <gtest/gtest.h>

TEST(YTDLPDownloadTest, TestInvalidVideo) {
  // getVideoInformationJSON("dfoisjfjd");
}

TEST(YTDLPDownloadTest, TestValidVideo) {
  // getVideoInformationJSON("https://www.youtube.com/watch?v=Zi_XLOBDo_Y");
}

TEST(YTDLPDownloadTest, TestDownloadValidVideo) {
  // download_video("https://www.youtube.com/watch?v=Zi_XLOBDo_Y", "240",
  // false);
}
