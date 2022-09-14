#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct VideoFormat {
  std::string format_id;
  std::string ext;
  std::optional<int> height;
  std::optional<int> width;
  std::string format_note;
  std::string url;
};

struct Thumbnail {
  std::string url;
  std::string id;
};

typedef std::optional<std::vector<Thumbnail>> Thumbnails;
struct VideoInformation {
  std::string url;
  std::string title;
  std::string uploader;
  std::string id;
  std::vector<VideoFormat> formats;
  Thumbnails thumbnails;
  std::string description;
  std::string extractor;
};

struct DownloadInfoRequest {
  std::string url;
  bool audio_only;
  std::string resolution;
  std::string operation = "download";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DownloadInfoRequest, url, audio_only,
                                   resolution, operation);

struct DownloadResponse {
  bool success;
  std::string error_msg;
  std::string file_path;
  std::string title;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DownloadResponse, success, error_msg,
                                   file_path);

struct InfoRequest {
  std::string url;
  std::string operation = "getInfo";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InfoRequest, url, operation)

struct InfoResponse {
  bool success;
  std::string error_msg;
  VideoInformation info;
};

DownloadResponse download_video(const std::string url,
                                const std::string resolution, bool audioOnly);

std::pair<std::shared_ptr<VideoInformation>, InfoResponse>
getVideoInformationJSON(const std::string url);

std::set<int> getAvailableResolutions(const VideoInformation &information);
std::string getThumbnail(const VideoInformation &information);
