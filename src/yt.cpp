#include "yt.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "utils.hpp"
#include <algorithm>
#include <array>
#include <boost/range/adaptor/reversed.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <variant>

const std::array<int, 8> RESOLUTIONS = {144, 240,  360,  480,
                                        720, 1080, 1440, 2160};

static std::regex formatNoteRegex("(^[\\d]+)");

template <typename T>
std::optional<T> get_at_optional(const nlohmann::json &obj,
                                 const std::string &key) try {
  return obj.at(key).get<T>();
} catch (...) {
  return std::nullopt;
}

template <typename T>
T value_at_or_default(const nlohmann::json &j, const char *key,
                      const T &default_value) {
  try {
    return j.at(key).get<T>();
  } catch (...) {
    return default_value;
  }
}

void from_json(const nlohmann::json &j, VideoFormat &p) {
  j.at("url").get_to(p.url);
  j.at("format_id").get_to(p.format_id);
  p.format_note = value_at_or_default<std::string>(j, "format_note", "unknown");
  j.at("ext").get_to(p.ext);
  p.height = get_at_optional<int>(j, "height");
  p.width = get_at_optional<int>(j, "width");
}

void from_json(const nlohmann::json &j, Thumbnail &p) {
  j.at("url").get_to(p.url);
  j.at("id").get_to(p.id);
}

void from_json(const nlohmann::json &j, VideoInformation &p) {
  j.at("id").get_to(p.id);
  p.description = value_at_or_default<std::string>(j, "description", "");
  j.at("extractor").get_to(p.extractor);
  j.at("filename").get_to(p.filename);
  p.thumbnails = get_at_optional<std::vector<Thumbnail>>(j, "thumbnails");
  j.at("formats").get_to<std::vector<VideoFormat>>(p.formats);
  p.uploader = value_at_or_default<std::string>(j, "uploader", "unknown");
  j.at("title").get_to(p.title);
  j.at("original_url").get_to(p.url);
}

#ifdef DEBUG
template class std::shared_ptr<VideoInformation>
#endif
    std::pair<std::shared_ptr<VideoInformation>, int>
    getVideoInformationJSON(const char *url) {
  auto [jsonString, exitCode] = exec("yt-dlp", {"-j", url});
  if (exitCode == 0) {
    try {
      auto json = nlohmann::json::parse(jsonString);
      auto videoFormat = json.get<VideoInformation>();
      return std::pair(std::make_shared<VideoInformation>(videoFormat),
                       exitCode);
    } catch (nlohmann::json::exception) {
      return std::pair(std::make_shared<VideoInformation>(), 1);
    }
  }

  return std::pair(std::make_shared<VideoInformation>(), exitCode);
}

std::set<int>
getAvailableResolutions(VideoInformation &information) {
  std::set<int> resolutions;
  if (information.formats.begin()->format_note == "unknown" &&
      information.extractor != "twitter") {
    return resolutions;
  }
  if (information.extractor == "youtube") {
    for (auto res : RESOLUTIONS) {
      for (auto &fmt : information.formats) {
        std::cmatch matches;

        if (std::regex_search(fmt.format_note.c_str(), matches,
                              formatNoteRegex))
          if (std::stoi(matches.begin()->str()) == res) {
            resolutions.insert(res);
          }
      }
    }
  } else {
    for (auto &i : information.formats) {
      if (min(i.width, i.height) >= 144) {
      }
    }
  }

  return resolutions;
}

std::string getThumbnail(VideoInformation &information) {
  if (!information.thumbnails.has_value())
    return "";
  for (auto &i : boost::adaptors::reverse(information.thumbnails.value())) {
    if (boost::algorithm::contains(i.url, ".webp")) {
      continue;
    }
    return i.url;
  }
}
