#pragma once
#include <cstdint>
#include <optional>
#include <sqlite3.h>
#include <string>

struct FileProviderFile {
  std::string title;
  std::string path;
  std::string urlHash;
  std::string filename;
  std::string id;
  std::string filePath;
  uint64_t lastAccessed;
};

void connectDatabase();
std::optional<FileProviderFile> getFile(const std::string &id);
std::optional<FileProviderFile> getFile(const std::string &url,
                                        const std::string &name);
std::string insertFile(const std::string &path, const std::string &url,
                       const std::string &title = "", bool isUrlHashed = false);
