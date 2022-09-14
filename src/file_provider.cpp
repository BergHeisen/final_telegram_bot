#include "file_provider.hpp"
#include "boost/filesystem/operations.hpp"
#include "plog/Log.h"
#include "utils.hpp"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <fmt/format.h>
#include <memory>
#include <openssl/sha.h>
#include <optional>
#include <sqlite3.h>
#include <sstream>
#include <string>

const std::string table_creation = R"(
  CREATE TABLE IF NOT EXISTS videos(
    id PRIMARY KEY,
    title TEXT NOT NULL,
    filename TEXT NOT NULL,
    path TEXT NOT NULL,
    urlHash TEXT NOT NULL,
    lastAccessed INT NOT NULL
  )
)";

int callback(void *data, int count, char **row, char **columns);

class SQLiteDatabase {
private:
  std::string path;
  sqlite3 *db;
  void connect() {
    open_db();
    close_db();
  }
  void open_db() { sqlite3_open(path.c_str(), &db); }
  void close_db() { sqlite3_close(db); }

  void createTable() {
    char *error_msg;
    const int exit =
        sqlite3_exec(db, table_creation.c_str(), NULL, NULL, &error_msg);
    if (exit != SQLITE_OK) {
      PLOGE << fmt::format("There was an error creating the table, errmsg: {}",
                           error_msg);
      sqlite3_free(error_msg);
    }
  }

public:
  SQLiteDatabase() {
    path = get_env_var_or_default("DOWNLOAD_FOLDER", "./");
    path.append("/repository.db");
    PLOGD << fmt::format("Using SQLite Database from {}", path);
    connect();
    open_db();
    createTable();
  }

  std::optional<FileProviderFile> getFile(const std::string &url,
                                          const std::string &filename) {
    std::optional<FileProviderFile> result;
    const std::string urlHash = hash(url);
    const char *query = sqlite3_mprintf(R"(
    SELECT * FROM videos
      WHERE filename = %Q AND urlHash = %Q
  )",
                                        filename.c_str(), urlHash.c_str());

    char *errmsg;
    sqlite3_exec(db, query, callback, &result, &errmsg);
    return result;
  }
  std::optional<FileProviderFile> getFile(const std::string &id) {
    std::optional<FileProviderFile> result;
    const std::string query =
        sqlite3_mprintf("SELECT * FROM Videos WHERE id = %Q", id.c_str());

    char *errmsg;
    sqlite3_exec(db, (query.c_str()), callback, &result, &errmsg);
    return result;
  }
  std::string insertFile(FileProviderFile &file) {
    PLOGD << fmt::format("Using SQLite Database from {}", path);
    const char *query = sqlite3_mprintf(
        "INSERT INTO videos(id, title, filename, path, urlHash, "
        "lastAccessed) VALUES(%Q, %Q, %Q, %Q, %Q, %Q)",
        file.id.c_str(), file.title.c_str(), file.filename.c_str(),
        file.path.c_str(), file.urlHash.c_str(),
        std::to_string(file.lastAccessed).c_str());
    char *errmsg;
    const int returnCode = sqlite3_exec(db, query, NULL, NULL, &errmsg);
    if (returnCode != SQLITE_OK) {
      PLOGE << fmt::format(
          "There was an issue inserting file path: {}, query: {},errmsg: {}",
          file.path, query, errmsg);
      sqlite3_free(errmsg);
    }
    return file.id;
  }
  std::string insertFile(const std::string &file_path, const std::string &url,
                         const std::string &title) {
    boost::filesystem::path p = boost::filesystem::absolute(file_path);
    boost::uuids::uuid id = boost::uuids::random_generator()();
    std::stringstream ss;
    ss << id;
    FileProviderFile file;
    file.filename = p.filename().string();
    file.title = title != "" ? title : p.stem().string();
    file.lastAccessed =
        std::chrono::system_clock::now().time_since_epoch().count();
    file.id = ss.str();
    file.path = p.parent_path().string();
    file.urlHash = hash(url);
    if (getFile(url, file.filename).has_value()) {
      PLOGD << fmt::format(
          "File path: {}/{} already existed, not inserting again", file.path,
          file.filename);
      return file.id;
    }

    return insertFile(file);
  }

  ~SQLiteDatabase() { close_db(); }
};

int callback(void *data, int count, char **row, char **columns) {
  std::optional<FileProviderFile> *result =
      (std::optional<FileProviderFile> *)data;
  FileProviderFile file;
  if (count == 0) {
    *result = std::nullopt;
    return 0;
  } else {
    for (int i = 0; i < count; ++i) {
      if (std::strcmp(columns[i], "id") == 0) {
        file.id = row[i];
      }
      if (std::strcmp(columns[i], "title") == 0) {
        file.title = row[i];
      }
      if (std::strcmp(columns[i], "filename") == 0) {
        file.filename = row[i];
      }
      if (std::strcmp(columns[i], "path") == 0) {
        file.path = row[i];
      }
      if (std::strcmp(columns[i], "urlHash") == 0) {
        file.urlHash = row[i];
      }
      if (std::strcmp(columns[i], "lastAccessed") == 0) {
        file.lastAccessed = std::stoull(row[i]);
      }
    }
    *result = std::make_optional(file);
  }
  return 0;
}
static SQLiteDatabase database;

std::optional<FileProviderFile> getFile(const std::string &id) {
  return database.getFile(id);
}
std::optional<FileProviderFile> getFile(const std::string &url,
                                        const std::string &name) {
  return database.getFile(url, name);
}

std::string insertFile(const std::string &file_path, const std::string &url,
                       const std::string &title) {

  return database.insertFile(file_path, url, title);
}
