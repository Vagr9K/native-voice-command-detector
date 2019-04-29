#include "VoiceManager.hpp"

VoiceManager::VoiceManager(AppConfig config, command_callback cb) {
  this->config = std::move(config);
  this->cb = std::move(cb);

  // Try to find the optimal worler thread amount
  auto num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    SPDLOG_WARN(
        "std::thread::hardware_concurrency returned 0 as an answer. Defaulting "
        "to 4 worker threads.");
    num_threads = 4;
  }

  // Create a thread pool
  pool = new ThreadPool(num_threads);
  SPDLOG_INFO("Detector started with {} worker threads.", num_threads);

  // Initialize CURL here, since otherwise we'll have thread safety issues
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Start the sync thread
  Ticker::Start();
}

VoiceManager::~VoiceManager() {
  // Clean up the thread pool
  delete pool;

  // Clean up the VoiceProcessors
  for (const auto& vp : vp_map) {
    delete vp.second;
  }

  // Cleanup CURL
  curl_global_cleanup();

  // Stop the sync thread
  Ticker::Stop();
}

void VoiceManager::AddOpusFrame(const std::string& id,
                                const opus_frame& frame) {
  // Try to find an existing VoiceProcessor via an ID from a Hash Map
  if (vp_map.find(id) == vp_map.end()) {
    // If not found, create a new one
    auto vp = new VoiceProcessor(id, config, pool, cb);
    vp->AddOpusFrame(frame);

    // Assign to the HashMap for the future reuse
    vp_map[id] = vp;
  } else {
    // Find an existing VoiceProcesor and push the new OPUS frames
    auto vp = vp_map[id];
    vp->AddOpusFrame(frame);
  }
}
