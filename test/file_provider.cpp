#include "../src/file_provider.hpp"
#include <gtest/gtest.h>

static std::string id;
TEST(FileProvider, insertFile) {
  id = insertFile("./ytdl_provider/videos/Zi_XLOBDo_Y_240.mp4",
                  "https://www.youtube.com/watch?v=Zi_XLOBDo_Y", "");
}

TEST(FileProvider, retrieveFile) {
  const auto file = getFile(id);
  ASSERT_EQ(id, file->id);
}

TEST(FileProvider, retrieveFileByFilename) {
  const auto file = getFile("https://www.youtube.com/watch?v=Zi_XLOBDo_Y",
                            "Zi_XLOBDo_Y_240.mp4");
  ASSERT_EQ(id, file->id);
}
