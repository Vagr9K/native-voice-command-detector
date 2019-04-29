#pragma once

#include <opus/opus.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include "../types.h"

// Decodes RAW OPUS frames into PCM
class OpusFrameDecoder {
 public:
  OpusFrameDecoder(int rate, int channels);
  ~OpusFrameDecoder();
  OpusFrameDecoder(const OpusFrameDecoder&) = delete;
  OpusFrameDecoder(const OpusFrameDecoder&&) = delete;

  // Decode the specified OPUS frames
  std::vector<pcm_frame> Decode(const std::vector<opus_frame>& opus_frames);

 private:
  // Decoder handle
  OpusDecoder* decoder = nullptr;
  // Decoder settings
  int channels;
  int rate;
};
