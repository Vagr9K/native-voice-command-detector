#pragma once

#include <ThreadPool.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "../Codecs/OpusDecoder.hpp"
#include "../Config/AppConfig.hpp"
#include "../Ticker/Ticker.hpp"
#include "../types.h"
#include "CommandProcessor.hpp"
#include "HotwordDetector.hpp"

// The instance of this class is responsible for processing the audio input of a
// single source
// It will also invoke a callback once the command speech is detected and parsed
// to text
class VoiceProcessor {
 public:
  VoiceProcessor(std::string id, AppConfig config, ThreadPool *pool,
                 command_callback cmd_callback);

  // Adds OPUS frames to the detection queue
  void AddOpusFrame(const std::vector<unsigned char> &frame);

 private:
  // Identifier
  std::string id;

  // Thread pool
  ThreadPool *pool;

  // Command callback
  command_callback cmd_callback;

  // App configuration
  AppConfig config;

  // Mutex for thread safety
  // All threads will be accessing this class instances
  std::mutex mt;

  // Data
  std::vector<opus_frame> opus_frames;
  std::vector<pcm_frame> pcm_frames;
  std::vector<CommandProcessor> command_segments;

  // State data
  int last_hotword_timestamp;
  int last_pcm_ready_timestamp;
  int last_opus_ready_timestamp;
  int last_pcm_data_timestamp;
  bool currently_processing_command = false;

  // Opus decoder
  OpusFrameDecoder decoder;

  // Hotword detector
  HotwordDetector detector;

  // Sync thread callback, that checks the VoiceProcessor state and invokes
  // processing based on it
  void OnSync();

  // Triggers OPUS buffer decoding
  void DecodeOPUS();
  // Triggers hotword detection on the PCM buffer
  void CheckForHotwords();
  // Wraps the text command callback with source ID and invokes the general
  // callback
  void CommandCallback(std::string &data);

  // Appends PCM fromes to the hotword detection queue after the encoding is
  // done
  void EnqueuePCMFrames(std::vector<pcm_frame> &new_pcm_frames);
  // Callback for when a hotword is detected
  void HotwordCallback(std::vector<pcm_frame> &leftover_pcm_frames);

  // Flushes and returns the existing OPUS buffer
  std::vector<opus_frame> FlushOpusFrames();

  // Flushes and returns the existing PCM buffer
  std::vector<pcm_frame> FlushPCMFrames();
};
