#include "converter.hpp"
#include "fmt/core.h"
#include "fmt/format.h"
#include "utils.hpp"
#include <map>
#include <memory>
#include <plog/Log.h>
#include <vector>

struct ConverterFactory {
  const std::map<const std::string, Converter *> converterMap = {
      //{"mp4", std::make_unique<MP4Converter>().get()},
      //{"mp3", std::make_unique<MP3Converter>().get()},
      //{"webm", std::make_unique<WEBMConverter>().get()},
      //{"gif", std::make_unique<GIFConverter>().get()},
      {"mp4", new MP4Converter()},
      {"mp3", new MP3Converter()},
      {"webm", new WEBMConverter()},
      {"gif", new GIFConverter()},

  };
  Converter *getConverter(const std::string &ext) {
    return converterMap.at(ext);
  }

  ~ConverterFactory() {
    for (auto [k, v] : converterMap) {
      delete converterMap.at(k);
    }
  }
};

Converter *getConverter(const std::string &extension) {
  static ConverterFactory factory;
  return factory.getConverter(extension);
}

int MP4Converter::convert(const std::string &path,
                          const std::string &out) const {
  const std::vector<std::string> args = {"-y",           "-i",      path,
                                         "-c:v",         "libx265", "-an",
                                         "-x265-params", "crf=25",  out};
  auto [output, statusCode] = exec("ffmpeg", args);
  if (statusCode != 0) {
    PLOGD << fmt::format("FFMpeg failed, statusCode: {}, args: output: {}",
                         statusCode, fmt::join(args.begin(), args.end(), " "),
                         statusCode);
  }
  return statusCode;
};

int MP3Converter::convert(const std::string &path,
                          const std::string &out) const {
  const std::vector<std::string> args = {
      "-y", "-i", path, "-vn", "-ar", "44100", "-ac", "2", "-b:a", "192k", out};
  auto [output, statusCode] = exec("ffmpeg", args);
  if (statusCode != 0) {
    PLOGD << fmt::format("FFMpeg failed, statusCode: {}, args: output: {}",
                         statusCode, fmt::join(args.begin(), args.end(), " "),
                         statusCode);
  }
  return statusCode;
};

int WEBMConverter::convert(const std::string &path,
                           const std::string &out) const {
  const std::vector<std::string> args = {
      "-y",      "-i",     path,        "-cpu-used", "1",
      "-vcodec", "libvpx", "-deadline", "realtime",  out};
  auto [output, statusCode] = exec("ffmpeg", args);
  if (statusCode != 0) {
    PLOGD << fmt::format("FFMpeg failed, statusCode: {}, args: output: {}",
                         statusCode, fmt::join(args.begin(), args.end(), " "),
                         statusCode);
  }
  return statusCode;
};

int GIFConverter::convert(const std::string &path,
                          const std::string &out) const {
  const std::vector<std::string> args = {
      "-y",
      "-i",
      path,
      "-vf",
      "\"fps=10,scale=320:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1]"
      "[p]paletteuse\"",
      out};
  auto [output, statusCode] = exec("ffmpeg", args);
  if (statusCode != 0) {
    PLOGD << fmt::format("FFMpeg failed, statusCode: {}, args: {} output: {}",
                         statusCode, fmt::join(args.begin(), args.end(), " "),
                         output);
  }
  return statusCode;
};
