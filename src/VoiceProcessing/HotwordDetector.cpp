#include "HotwordDetector.hpp"

HotwordDetector::HotwordDetector(
    std::string& keyword_path, std::string& model_path, float sensitivity,
    std::function<void(std::vector<pcm_frame>&)> callback) {
  this->callback = std::move(callback);
  SPDLOG_INFO("Initializing porcupine hotword detector.");

  SPDLOG_INFO(
      "Sensitivity: {}, keyword "
      "path: {}, model path: {}.",
      sensitivity, keyword_path, model_path);

  // Initialize Porcupine and gather the buffer parsing settings
  pv_status_t status = pv_porcupine_init(
      model_path.c_str(), keyword_path.c_str(), sensitivity, &porcupine_object);
  if (status != PV_STATUS_SUCCESS) {
    SPDLOG_ERROR("Failed to initialize Porcupine.");
  }

  pv_frame_buffer_size = pv_porcupine_frame_length();
};

HotwordDetector::~HotwordDetector() { pv_porcupine_delete(porcupine_object); }

void HotwordDetector::Check(std::vector<pcm_frame> pcm_data) {
  // Prevent concurrent checks
  std::lock_guard<std::mutex> lck(mt);
  // Add to the main buffer in case there are any leftover frames
  buffer.insert(buffer.end(), pcm_data.begin(), pcm_data.end());

  SPDLOG_DEBUG("HotwordDetector::Check : In progress. pcm_data size: {}.",
               pcm_data.size());
  bool detected = false;

  // Check frames in batches according to pv_frame_buffer_size
  while (!detected && buffer.size() > pv_frame_buffer_size) {
    pv_porcupine_process(porcupine_object, &buffer[0], &detected);

    if (detected) {
      SPDLOG_INFO(
          "HotwordDetector::Check : Keyword detected. Remaining buffer "
          "size: {}.",
          buffer.size());

      // Once a hotword is detected, submit the remaining audio data to the
      // callback
      auto leftover_buffer = buffer;
      buffer.clear();
      this->callback(leftover_buffer);
    } else {
      // Remove the already checked audio data from the buffer
      buffer.erase(buffer.begin(), buffer.begin() + pv_frame_buffer_size);

      SPDLOG_DEBUG(
          "HotwordDetector::Check : No keyword detected. Remaining buffer "
          "size: {}.",
          buffer.size());
    }
  }
}
