#pragma once
#include <memory>
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
  std::string title;
  std::string uploader;
  std::string id;
  std::string filename;
  std::vector<VideoFormat> formats;
  Thumbnails thumbnails;
  std::string description;
  std::string extractor;
};

int download_video(const char *url, const char *resolution,
                   const char *output_folder);

std::pair<std::shared_ptr<VideoInformation>, int>
getVideoInformationJSON(const char *url);

std::set<std::string> getAvailableResolutions(VideoInformation &information);
