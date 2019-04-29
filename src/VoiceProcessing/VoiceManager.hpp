#include <ThreadPool.h>
#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>
#include "../Config/AppConfig.hpp"
#include "../Ticker/Ticker.hpp"
#include "../types.h"
#include "VoiceProcessor.hpp"

// Manages all the VoiceProcessor instances and the threadpool used for the
// processing tasks
class VoiceManager {
 public:
  VoiceManager(AppConfig config, command_callback cb);
  VoiceManager(const VoiceManager&) = delete;
  VoiceManager(const VoiceManager&&) = delete;
  ~VoiceManager();

  // Adds an OPUS frame to the voice processing queue
  void AddOpusFrame(const std::string& id, const opus_frame& frame);

 private:
  // Hashmap to store all the VoiceProcessor instance pointers
  std::unordered_map<std::string, VoiceProcessor*> vp_map;
  // Threadpool handle
  ThreadPool* pool;
  // N-API callback
  command_callback cb;
  // Applciation wide configuration
  AppConfig config;
};
