#include "CommandProcessor.hpp"

CommandProcessor::CommandProcessor(
    AppConfig config, const std::shared_ptr<ThreadPool>& pool,
    std::function<void(std::string&)> data_callback)
    : pool(pool), is_done(false) {
  this->config = std::move(config);
  this->data_callback = std::move(data_callback);
};

// Add audio to the storage buffer
void CommandProcessor::AddAudio(std::vector<pcm_frame>& frames) {
  command_pcm_frames.insert(command_pcm_frames.end(), frames.begin(),
                            frames.end());

  SPDLOG_DEBUG(
      "CommandProcessor::AddAudio : New frames: {}, current buffer size is {}.",
      frames.size(), command_pcm_frames.size());
}

void CommandProcessor::StartProcessing() {
  // Enqueue a task for the threadpool
  pool->enqueue([this]() {
    std::function<void(std::vector<unsigned char>&)> cb =
        [this](std::vector<unsigned char>& encoded_ogg_opus) {
          GSpeechToText parser(this->config.g_speech_to_text_api_key);

          SPDLOG_INFO(
              "CommandProcessor::StartProcessing::encoded_ogg_opus_cb : "
              "encoded_ogg_opus size is {}.",
              encoded_ogg_opus.size());

          // Invoke speech to text parsing
          auto json_data = parser.GetTextFromOggOpus(encoded_ogg_opus);

          // Parse the output and select the most likely correct result
          auto parsed_data = nlohmann::json::parse(json_data);
          std::string data =
              parsed_data["results"][0]["alternatives"][0]["transcript"];

          SPDLOG_INFO(
              "CommandProcessor::StartProcessing::encoded_ogg_opus_cb : "
              "Finished "
              "parsing speech.");

          SPDLOG_DEBUG(
              "CommandProcessor::StartProcessing::encoded_ogg_opus_cb : Text "
              "data "
              "is: {}",
              data);

          // Callback VoiceProcessor
          this->data_callback(data);

          // Set as done for later cleanup
          this->is_done = true;
        };

    // Encode the PCM frames in OggOpus format
    // Callback for further processing
    OpusOggEncoder encoder(cb);
    encoder.Encode(this->command_pcm_frames);
  });
}

bool CommandProcessor::GetStatus() { return is_done; };
