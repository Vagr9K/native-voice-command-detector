#pragma once

#include <ThreadPool.h>
#include <base64.h>
#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "../APIs/GSpeechToText.hpp"
#include "../Codecs/OpusOggEncoder.hpp"
#include "../Config/AppConfig.hpp"
#include "../types.h"

// Stores the command releted audio and transforms it into a text command
class CommandProcessor {
 public:
  CommandProcessor(AppConfig config, ThreadPool* pool,
                   std::function<void(std::string&)> data_callback);
  // Add audio to the storage buffer
  void AddAudio(std::vector<pcm_frame>& frames);

  // Start the speech to text conversion
  void StartProcessing();

 private:
  // PCM buffer for the audio segment storage
  std::vector<pcm_frame> command_pcm_frames;

  // Application wide configuration
  AppConfig config;

  // Callback for when the text data is ready
  std::function<void(std::string&)> data_callback;

  // Thread pool
  ThreadPool* pool;
};
