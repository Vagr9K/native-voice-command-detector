#include "OpusDecoder.hpp"

// OPUS decoder constants
constexpr int max_frame_size = 6 * 960;

OpusFrameDecoder::OpusFrameDecoder(int rate, int channels)
    : channels(channels), rate(rate) {
  int err;
  decoder = opus_decoder_create(rate, channels, &err);

  if (err != OPUS_OK) {
    throw std::runtime_error("opus_decoder_create failed");
  }
};
OpusFrameDecoder::~OpusFrameDecoder() { opus_decoder_destroy(decoder); }

// Decode the specified OPUS frames
std::vector<pcm_frame> OpusFrameDecoder::Decode(
    const std::vector<opus_frame>& opus_frames) {
  // Final return buffer
  std::vector<opus_int16> pcm_buffer;
  // Intermediate buffer for decoded output
  // NOLINTNEXTLINE
  opus_int16 pcm_output[channels * max_frame_size];
  // Intermediate buffer for bit manipulation
  // NOLINTNEXTLINE
  unsigned char pcm_bytes[max_frame_size * channels * 2];

  // Decode frame by frame
  for (auto frame : opus_frames) {
    int decoded_samples = opus_decode(decoder, frame.data(), frame.size(),
                                      pcm_output, max_frame_size,
                                      /* decode_fex */ 0);

    if (decoded_samples > 0) {
      // Convert to little-endian ordering
      for (int i = 0; i < channels * decoded_samples; i++) {
        pcm_bytes[2 * i] = pcm_output[i] & 0xFF;             // NOLINT
        pcm_bytes[2 * i + 1] = (pcm_output[i] >> 8) & 0xFF;  // NOLINT
      }

      // Move back into an int16 format
      size_t frame_buffer_size = channels * decoded_samples * 2 /
                                 2;  // NOTE: BYTE->INT16, hence half the size
      auto frame_buffer = reinterpret_cast<int16_t*>(pcm_bytes);

      // Add to the main buffer
      pcm_buffer.insert(pcm_buffer.end(), frame_buffer,
                        frame_buffer + frame_buffer_size);
    } else {
      SPDLOG_ERROR("Failed to decode an Opus frame.");
    }
  }

  return pcm_buffer;
}
