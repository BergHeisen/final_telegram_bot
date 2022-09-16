#pragma once
#include <memory>
#include <string>

class Converter {
public:
  virtual int convert(const std::string &path,
                      const std::string &out) const = 0;
  virtual const std::string getExtension() const = 0;
};

class MP4Converter : public Converter {
public:
  int convert(const std::string &path, const std::string &out) const override;
  MP4Converter() = default;
  const std::string getExtension() const override { return "mp4"; };
  ~MP4Converter() = default;
};

class MP3Converter : public Converter {
public:
  int convert(const std::string &path, const std::string &out) const override;
  MP3Converter() = default;
  const std::string getExtension() const override { return "mp3"; };
  ~MP3Converter() = default;
};

class GIFConverter : public Converter {
public:
  int convert(const std::string &path, const std::string &out) const override;
  GIFConverter() = default;
  const std::string getExtension() const override { return "gif"; };
  ~GIFConverter() = default;
};

class WEBMConverter : public Converter {
public:
  int convert(const std::string &path, const std::string &out) const override;
  WEBMConverter() = default;
  const std::string getExtension() const override { return "webm"; };
  ~WEBMConverter() = default;
};

Converter *getConverter(const std::string &extension);
