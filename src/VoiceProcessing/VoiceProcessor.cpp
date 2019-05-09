#include "VoiceProcessor.hpp"

// Unnamed namespace for local utilities
namespace {
int GetCurrentTimeStampInMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
}  // namespace

// Audio decoding settings
constexpr int audio_rate = 16000;
constexpr int audio_channels = 1;

VoiceProcessor::VoiceProcessor(std::string id, AppConfig config,
                               const std::shared_ptr<ThreadPool> &pool,
                               command_callback cmd_callback)
    : pool(pool),
      detector(config.pv_keyword_path, config.pv_model_path,
               config.pv_sensitivity,
               std::bind(&VoiceProcessor::HotwordCallback, this,
                         std::placeholders::_1)),
      last_opus_ready_timestamp(GetCurrentTimeStampInMs()),
      last_pcm_ready_timestamp(GetCurrentTimeStampInMs()),
      last_hotword_timestamp(GetCurrentTimeStampInMs()),
      last_pcm_data_timestamp(GetCurrentTimeStampInMs()),
      decoder(audio_rate, audio_channels)

{
  this->id = std::move(id);
  this->config = std::move(config);

  // Callback for command text if detected
  this->cmd_callback = std::move(cmd_callback);

  // Register a callback for the sync thread
  Ticker::RegisterCallback(std::bind(&VoiceProcessor::OnSync, this));
}

void VoiceProcessor::AddOpusFrame(const std::vector<unsigned char> &frame) {
  std::lock_guard<std::mutex> lk(mt);

  // Add frames to the opus decoding queue
  opus_frames.push_back(frame);
}

void VoiceProcessor::OnSync() {
  std::lock_guard<std::mutex> lk(mt);

  SPDLOG_TRACE("VoiceProcessor::OnSync : Invoked for ID:{}.", id);

  const int current_time = GetCurrentTimeStampInMs();

  SPDLOG_TRACE(
      "VoiceProcessor::OnSync : opus_frames: {}, pcm_frames: {}, "
      "command_segments: {}.",
      opus_frames.size(), pcm_frames.size(), command_segments.size());

  // Check OPUS buffer timeouts
  if (!opus_frames.empty()) {
    if (current_time - last_opus_ready_timestamp > config.max_buffer_ttl_ms) {
      SPDLOG_DEBUG("VoiceProcessor::OnSync : Triggering DecodeOPUS.");
      DecodeOPUS();
    }
  } else {
    // If the buffer is empty, reset the timestamp as this should be treated
    // as a "silence" input
    last_opus_ready_timestamp = current_time;
  }

  // Check PCM buffer timeouts
  if (!pcm_frames.empty()) {
    if (current_time - last_pcm_ready_timestamp > config.max_buffer_ttl_ms) {
      SPDLOG_DEBUG("VoiceProcessor::OnSync : Triggering CheckForHotwords.");
      CheckForHotwords();
    }
  } else {
    // If the buffer is empty, reset the timestamp as this should be treated
    // as a "silence" input
    last_pcm_ready_timestamp = current_time;
  }

  // Check if we hit the time limit for a command
  if (currently_processing_command) {
    if (current_time - last_hotword_timestamp > config.max_command_length_ms) {
      SPDLOG_INFO(
          "VoiceProcessor::OnSync : Triggering "
          "CommandSegment->StartProcessing().");
      // Set as not processing
      currently_processing_command = false;
      // Set command segment as ready and process
      command_segments.back()->StartProcessing();

    } else if (current_time - last_pcm_data_timestamp >
               config.max_command_silence_length_ms) {
      SPDLOG_INFO(
          "VoiceProcessor::OnSync : Triggering "
          "CommandSegment->StartProcessing() "
          "due "
          "to silence.");
      // Set as not processing
      currently_processing_command = false;
      // Set command segment as ready and process
      command_segments.back()->StartProcessing();
    }
  } else {
    // If we're not procesing a command, reset the timestamp as this should be
    // treated as a "pending" input
    last_hotword_timestamp = current_time;
  }

  // Cleanup old redundant CommandProcessor entries
  for (auto i = command_segments.begin(); i != command_segments.end();) {
    if ((*i)->GetStatus()) {
      i = command_segments.erase(i);
    } else {
      i++;
    }
  }
}

void VoiceProcessor::DecodeOPUS() {
  // Only called from the sync thread that already has a lock acquired

  // Set the updated timestamp
  last_opus_ready_timestamp = GetCurrentTimeStampInMs();

  // Enqueue a task for the threadpool to process
  // Docode OPUS frames into PCM and append to the buffer
  pool->enqueue([this]() {
    auto opus_frames = this->FlushOpusFrames();
    auto pcm_buffer = this->decoder.Decode(opus_frames);
    this->EnqueuePCMFrames(pcm_buffer);
  });
}

void VoiceProcessor::CheckForHotwords() {
  // Only called from the sync thread that already has a lock acquired

  // Set the updated timestamp
  last_pcm_ready_timestamp = GetCurrentTimeStampInMs();

  // Enqueue a task for the threadpool to process
  // Check the PCM audio data for hotwords
  pool->enqueue([this]() {
    auto pcm_data = this->FlushPCMFrames();
    detector.Check(pcm_data);
  });
}

void VoiceProcessor::CommandCallback(std::string &data) {
  // Wrap the text command callback with source ID and invoke the general
  // callback
  cmd_callback(id, data);
}

void VoiceProcessor::EnqueuePCMFrames(std::vector<pcm_frame> &new_pcm_frames) {
  std::lock_guard<std::mutex> lk(mt);

  // Update timestamp
  last_pcm_data_timestamp = GetCurrentTimeStampInMs();

  // If a command is being currently processed, also append to that command
  // processor
  if (currently_processing_command) {
    command_segments.back()->AddAudio(new_pcm_frames);
  }
  // Add to the hotword detection queue
  pcm_frames.insert(pcm_frames.end(), new_pcm_frames.begin(),
                    new_pcm_frames.end());
}

void VoiceProcessor::HotwordCallback(
    std::vector<pcm_frame> &leftover_pcm_frames) {
  std::lock_guard<std::mutex> lk(mt);

  SPDLOG_DEBUG("VoiceProcessor::HotwordCallback : Invoked.");

  // If currently processing another command, register it as ready
  if (currently_processing_command) {
    command_segments.back()->StartProcessing();

    SPDLOG_DEBUG(
        "VoiceProcessor::HotwordCallback : Setting last command processor "
        "as ready.");

  } else {
    // Register the fact that we've detected a hotword
    currently_processing_command = true;

    SPDLOG_DEBUG(
        "VoiceProcessor::HotwordCallback : Registered in "
        "currently_processing_command.");
  }

  // Set the timestamp
  last_hotword_timestamp = GetCurrentTimeStampInMs();

  // Create a full audio buffer from existing and leftover pcm frames
  std::vector<pcm_frame> full_pcm_buffer;
  full_pcm_buffer.insert(full_pcm_buffer.end(), leftover_pcm_frames.begin(),
                         leftover_pcm_frames.end());
  full_pcm_buffer.insert(full_pcm_buffer.end(), pcm_frames.begin(),
                         pcm_frames.end());

  // Add a new command segment
  auto new_command_processor = std::make_shared<CommandProcessor>(
      config, pool,
      std::bind(&VoiceProcessor::CommandCallback, this, std::placeholders::_1));

  new_command_processor->AddAudio(full_pcm_buffer);
  command_segments.push_back(std::move(new_command_processor));

  SPDLOG_DEBUG(
      "VoiceProcessor::HotwordCallback : New command processor added.");
}

std::vector<opus_frame> VoiceProcessor::FlushOpusFrames() {
  std::lock_guard<std::mutex> lk(mt);

  // Move to prevent copy constructor invocation and then reset the variable
  // state via clear()
  auto ret = std::move(opus_frames);
  opus_frames.clear();

  return ret;
}

std::vector<pcm_frame> VoiceProcessor::FlushPCMFrames() {
  std::lock_guard<std::mutex> lk(mt);

  // Move to prevent copy constructor invocation and then reset the variable
  // state via clear()
  const auto ret = std::move(pcm_frames);
  pcm_frames.clear();

  return ret;
}
