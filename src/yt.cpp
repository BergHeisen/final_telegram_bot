#include "yt.hpp"
#include "utils.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <set>
#include <utility>
#include <variant>

const std::array<std::string, 8> RESOLUTIONS = {"144", "240",  "360",  "480",
                                                "720", "1080", "1440", "2160"};

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
}

std::pair<std::shared_ptr<VideoInformation>, int>
getVideoInformationJSON(const char *url) {
  auto [jsonString, exitCode] = exec("yt-dlp", {"-j", url});
  if (exitCode == 0) {
    auto json = nlohmann::json::parse(jsonString);
    auto videoFormat = json.get<VideoInformation>();
    return std::pair(std::make_shared<VideoInformation>(videoFormat), exitCode);
  }

  return std::pair(std::make_shared<VideoInformation>(), exitCode);
}

std::set<std::string> getAvailableResolutions(VideoInformation &information) {
  std::set<std::string> resolutions;
  if (information.formats.begin()->format_note == "unknown") {
    return resolutions;
  }
  /*if (information.extractor == "youtube") {
    std::copy_if(RESOLUTIONS.begin(), RESOLUTIONS.end(),
                 std::back_inserter(resolutions), [&](std::string &res) {
                   std::any_of(information.formats.begin(),
                               information.formats.end(),
                               [&](VideoFormat &fmt) {
                                 std::cmatch matches;
                                 std::regex_search(fmt.format_note.c_str(),
                                                   matches, formatNoteRegex);
                                 if (matches.size() == 0)
                                   return false;
                                 return matches.begin()->str() == res;
                               });
                 });
    return resolutions;
  }
  std::copy_if(information.formats.begin(), information.formats.end(),
               std::back_inserter(resolutions), [](VideoFormat &fmt) {
                 return std::min(fmt.height, fmt.width);
               });*/
  for (auto &i : information.formats) {
    if (min(i.width, i.height) >= 144) {
      resolutions.insert(std::to_string(min(i.width, i.height).value()));
    }
  }
  return resolutions;
}
