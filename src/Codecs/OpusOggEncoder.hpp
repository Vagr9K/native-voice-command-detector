#pragma once

#include <opus/opus.h>
#include <opus/opusenc.h>
#include <functional>
#include <string>
#include <vector>
#include "../types.h"

// Encoder PCM to OggOpus
class OpusOggEncoder {
 public:
  explicit OpusOggEncoder(std::function<void(std::vector<unsigned char> &)> cb);

  // Encodes the specified frames as OggOpus
  void Encode(const std::vector<pcm_frame> &pcm_frames);

 private:
  // Callback for when the encoding is done
  std::function<void(std::vector<unsigned char> &)> cb;
  // Buffer to store the encoded data
  std::vector<unsigned char> enc_buffer;

  // Invoked as a callback when the partially encoded data is ready
  void AddToEncodedDataBuffer(const unsigned char *ptr, opus_int32 len);

  // Invoked as a callback when the encoding is done
  void OnEncodingDone();
};
