#include "OpusOggEncoder.hpp"

// Encoder configuration
constexpr int channels = 1;
constexpr int rate = 16000;

OpusOggEncoder::OpusOggEncoder(
    std::function<void(std::vector<unsigned char> &)> cb) {
  this->cb = std::move(cb);
};

// Encodes the specified frames as OggOpus
void OpusOggEncoder::Encode(const std::vector<pcm_frame> &pcm_frames) {
  OggOpusEnc *enc;
  OggOpusComments *comments;

  // OpusEnc will invoke these callbacks during the encode process
  OpusEncCallbacks callbacks = {
      // Callback with encoded data for an OggOpus page
      [](void *user_data, const unsigned char *ptr, opus_int32 len) {
        auto inst = static_cast<OpusOggEncoder *>(user_data);
        inst->AddToEncodedDataBuffer(ptr, len);
        return 0;
      },
      // Callback for notifying about the end of encoding
      [](void *user_data) {
        auto inst = static_cast<OpusOggEncoder *>(user_data);
        inst->OnEncodingDone();
        return 0;
      }};
  int error;

  // Create empty comments
  comments = ope_comments_create();

  // Initialize the encoder
  enc = ope_encoder_create_callbacks(&callbacks, this, comments, rate, channels,
                                     0, &error);
  if (!enc) {
    ope_comments_destroy(comments);
    throw std::runtime_error("Failed ope_encoder_create_callbacks");
  }

  // Add the data to the encoder and drain it
  ope_encoder_write(enc, pcm_frames.data(), pcm_frames.size());
  ope_encoder_drain(enc);

  // Cleanup
  ope_comments_destroy(comments);
  ope_encoder_destroy(enc);
}

void OpusOggEncoder::AddToEncodedDataBuffer(const unsigned char *ptr,
                                            opus_int32 len) {
  // Append the partial data to the buffer
  enc_buffer.insert(enc_buffer.end(), ptr, ptr + len);
}

void OpusOggEncoder::OnEncodingDone() {
  // Callback the full data
  cb(enc_buffer);
}
