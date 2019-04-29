#include "CommandProcessor.hpp"

CommandProcessor::CommandProcessor(
    AppConfig config, ThreadPool* pool,
    std::function<void(std::string&)> data_callback)
    : pool(pool) {
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
  auto pcm_frames = command_pcm_frames;
  auto g_cloud_api_key = config.g_speech_to_text_api_key;
  std::function<void(std::string&)> text_data_callback = data_callback;

  // Enqueue a task for the threadpool
  pool->enqueue([pcm_frames, text_data_callback, g_cloud_api_key]() {
    std::function<void(std::vector<unsigned char>&)> cb =
        [&text_data_callback,
         &g_cloud_api_key](std::vector<unsigned char>& encoded_ogg_opus) {
          GSpeechToText parser(g_cloud_api_key);

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
          text_data_callback(data);
        };

    // Encode the PCM frames in OggOpus format
    // Callback for further processing
    OpusOggEncoder encoder(cb);
    encoder.Encode(pcm_frames);
  });
}
