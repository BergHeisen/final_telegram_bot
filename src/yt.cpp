#include "yt.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "request_handler.hpp"
#include "tcp_client.hpp"
#include "utils.hpp"
#include <algorithm>
#include <array>
#include <boost/range/adaptor/reversed.hpp>
#include <cstddef>
#include <cstring>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <plog/Log.h>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <variant>

const std::array<int, 8> RESOLUTIONS = {144, 240,  360,  480,
                                        720, 1080, 1440, 2160};

const std::string HOST = get_env_var_or_default("YTDL_HOST", "127.0.0.1");
const std::string PORT = get_env_var_or_default("YTDL_HOST_PORT", "3436");

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

struct Stream {
  std::string codec_type;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Stream, codec_type);

struct FFProbeAudioRequestResult {
  std::vector<Stream> streams;
};

void from_json(const nlohmann::json &j, FFProbeAudioRequestResult &p) {
  j.at("streams").get_to<std::vector<Stream>>(p.streams);
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
  p.thumbnails = get_at_optional<std::vector<Thumbnail>>(j, "thumbnails");
  j.at("formats").get_to<std::vector<VideoFormat>>(p.formats);
  p.uploader = value_at_or_default<std::string>(j, "uploader", "unknown");
  j.at("title").get_to(p.title);
  j.at("original_url").get_to(p.url);
}

void from_json(const nlohmann::json &j, InfoResponse &p) {
  j.at("success").get_to(p.success);
  j.at("error_msg").get_to(p.error_msg);
  p.info = value_at_or_default(j, "info", VideoInformation{});
}

std::pair<std::shared_ptr<VideoInformation>, InfoResponse>
getVideoInformationJSON(const std::string url) {
  auto connection = TCPConnection::instantiate(HOST, std::stoi(PORT));
  nlohmann::json jR;
  InfoRequest r{url};
  to_json(jR, r);
  std::string jsonString = connection->exchangeMessage(jR.dump());
  LOGD << fmt::format("jsonString: {}", jsonString);
  nlohmann::json jA = nlohmann::json::parse(jsonString);
  auto infoResponse = jA.get<InfoResponse>();
  if (infoResponse.success == false) {
    LOGD << fmt::format("GetInfo on URL {} failed, error message: {}", url,
                        infoResponse.error_msg);
    return std::pair(std::make_shared<VideoInformation>(), infoResponse);
  } else {
    auto info = std::make_shared<VideoInformation>(infoResponse.info);
    return std::pair(info, infoResponse);
  }
}

std::set<int> getAvailableResolutions(const VideoInformation &information) {
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

std::string getThumbnail(const VideoInformation &information) {
  if (!information.thumbnails.has_value())
    return "";
  for (auto &i : boost::adaptors::reverse(information.thumbnails.value())) {
    if (boost::algorithm::contains(i.url, ".webp")) {
      continue;
    }
    return i.url;
  }
}

DownloadResponse download_video(const std::string url,
                                const std::string resolution, bool audioOnly) {
  auto connection = TCPConnection::instantiate(HOST, std::stoi(PORT));
  nlohmann::json jR;
  DownloadInfoRequest r{url, audioOnly, resolution};
  to_json(jR, r);
  std::string jsonString = connection->exchangeMessage(jR.dump());
  LOGD << fmt::format("jsonString: {}", jsonString);
  nlohmann::json jA = nlohmann::json::parse(jsonString);
  auto infoResponse = jA.get<DownloadResponse>();
  if (infoResponse.success == false) {
    LOGD << fmt::format("GetInfo on URL {} failed, error message: {}", url,
                        infoResponse.error_msg);
    return infoResponse;
  } else {
    return infoResponse;
  }
}

bool fileHasAudio(const std::string &filePath) {
  auto [output, statusCode] =
      exec("ffprobe", {"-loglevel", "error", "-show_entries",
                       "stream=codec_type", "-of", "json", filePath});
  if (statusCode != 0)
    return false;
  try {
    nlohmann::json j = nlohmann::json::parse(output);
    const auto result = j.get<FFProbeAudioRequestResult>();
    for (auto &i : result.streams) {
      if (i.codec_type == "audio")
        return true;
    }
    return false;
  } catch (...) {
    return false;
  }
}
