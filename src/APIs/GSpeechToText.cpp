#include "GSpeechToText.hpp"

GSpeechToText::GSpeechToText(std::string api_key) {
  this->api_key = std::move(api_key);
}

std::string GSpeechToText::GetTextFromOggOpus(
    const std::vector<unsigned char>& audio_data) {
  std::string api_url =
      "https://speech.googleapis.com/v1/speech:recognize?key=" + api_key;
  std::string payload = GetOggAudioPayload(audio_data);

  return HTTPClient::PostJson(api_url, payload);
}

std::string GSpeechToText::GetOggAudioPayload(
    std::vector<unsigned char> audio_data) {
  // Setup the payload
  nlohmann::json payload;
  payload["config"]["audioChannelCount"] = 1;
  payload["config"]["encoding"] = "OGG_OPUS";
  payload["config"]["model"] = "command_and_search";
  payload["config"]["enableAutomaticPunctuation"] = false;
  payload["config"]["sampleRateHertz"] = 16000;
  payload["config"]["languageCode"] = "en-US";
  payload["config"]["enableWordTimeOffsets"] = true;

  // B64 encode the audio data and add to the payload
  std::string b64_audio = base64_encode(audio_data.data(), audio_data.size());
  payload["audio"]["content"] = b64_audio;

  // Return the stringified JSON
  return payload.dump();
}
