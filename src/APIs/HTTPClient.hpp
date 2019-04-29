#pragma once

#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include "../Codecs/OpusOggEncoder.hpp"

// A CURL wrapper to perform API calls with
class HTTPClient {
 public:
  static std::string PostJson(const std::string& uri,
                              const std::string& json_data);
};
