#pragma once
#include <string>

// Stores application configuration
class AppConfig {
 public:
  std::string pv_model_path;
  std::string pv_keyword_path;
  float pv_sensitivity;
  std::string g_speech_to_text_api_key;
  int max_buffer_ttl_ms;
  int max_command_length_ms;
  int max_command_silence_length_ms;
};
