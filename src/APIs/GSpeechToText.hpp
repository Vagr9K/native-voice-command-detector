#pragma once

#include <base64.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "HTTPClient.hpp"

// Google Cloud Text To Speech API wrapper
class GSpeechToText {
 public:
  explicit GSpeechToText(std::string api_key);
  // Makes the GCloud API call to get the text of of speech
  std::string GetTextFromOggOpus(const std::vector<unsigned char>& audio_data);

 private:
  // API key to use
  std::string api_key;
  // Generates a JSON payload for querying the GCloud API
  std::string GetOggAudioPayload(std::vector<unsigned char> audio_data);
};
