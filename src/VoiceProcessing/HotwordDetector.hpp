#pragma once

#include <ThreadPool.h>
#include <picovoice.h>
#include <pv_porcupine.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "../types.h"

// Processes audio and detects the hotwords
// Needs to be VoiceProcessor specific since it holds lefotover buffers
class HotwordDetector {
 public:
  HotwordDetector(std::string& keyword_path, std::string& model_path,
                  float sensitivity,
                  std::function<void(std::vector<pcm_frame>&)> callback);
  HotwordDetector(const HotwordDetector&) = delete;
  HotwordDetector(const HotwordDetector&&) = delete;
  ~HotwordDetector();

  // Checks the data for hotwords
  void Check(std::vector<pcm_frame> pcm_data);

 private:
  // Porcupine handles/data
  pv_porcupine_object_t* porcupine_object = nullptr;
  size_t pv_frame_buffer_size;

  // Invoked on hotword detection
  std::function<void(std::vector<pcm_frame>&)> callback;

  // Buffer to store the PCM data and leftovers in
  std::vector<pcm_frame> buffer;
};
