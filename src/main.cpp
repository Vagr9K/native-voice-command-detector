#include <napi.h>
#include <functional>
#include <memory>
#include <napi-thread-safe-callback.hpp>
#include <string>
#include <vector>
#include "Config/AppConfig.hpp"
#include "Utils/LogSetup.hpp"
#include "VoiceProcessing/VoiceManager.hpp"
#include "types.h"

// Accessed only from the main thread
// No locks needed
class Detector : public Napi::ObjectWrap<Detector> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func =
        DefineClass(env, "Detector",
                    {InstanceMethod("addOpusFrame", &Detector::AddOpusFrame)});

    exports.Set("Detector", func);
    return exports;
  }

  // Constructor for the JS class
  explicit Detector(const Napi::CallbackInfo& info)
      : Napi::ObjectWrap<Detector>(info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int arg_count = info.Length();

    if (arg_count < 8) {
      std::string error =
          "8 arguments expected. Provided " + std::to_string(arg_count) + ".";
      Napi::TypeError::New(env, error).ThrowAsJavaScriptException();
    }

    // Arguments are:
    // PV model file path
    // PV keyword file path
    // PV sensitivity
    // GCloud Speech To Text API key
    // Max buffer TTL (ms)
    // Max command audio length (ms)
    // Max silence length when parsing a command
    // Callback for receiving command text data

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsNumber() ||
        !info[3].IsString() || !info[4].IsNumber() || !info[5].IsNumber() ||
        !info[6].IsNumber() || !info[7].IsFunction()) {
      Napi::TypeError::New(
          env,
          "Wrong arguments. Expected pv_model_path:string, pv_keyword_path: "
          "string, pv_sensitivity: double, g_speech_to_text_api_key: string, "
          "max_buffer_ttl_ms: int, max_command_length_ms: int, "
          "max_command_silence_length_ms: int, "
          "callback: function.")
          .ThrowAsJavaScriptException();
    }

    // Get arguments
    config.pv_model_path = info[0].ToString();
    config.pv_keyword_path = info[1].ToString();
    config.pv_sensitivity = info[2].ToNumber().FloatValue();
    config.g_speech_to_text_api_key = info[3].ToString();
    config.max_buffer_ttl_ms = info[4].ToNumber().Int32Value();
    config.max_command_length_ms = info[5].ToNumber().Int32Value();
    config.max_command_silence_length_ms = info[6].ToNumber().Int32Value();

    // Initialize VoiceManager
    voice_manager = new VoiceManager(
        config, std::bind(&Detector::SendCommand, this, std::placeholders::_1,
                          std::placeholders::_2));

    // Create a thread safe callback function
    this->node_callback =
        std::make_shared<ThreadSafeCallback>(info[7].As<Napi::Function>());
  }

  Detector(const Detector&) = delete;
  Detector(const Detector&&) = delete;

  ~Detector() { delete voice_manager; }

 private:
  std::shared_ptr<ThreadSafeCallback> node_callback;
  VoiceManager* voice_manager;
  AppConfig config;

  // Adds an Opus frame to the buffer
  void AddOpusFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
      Napi::TypeError::New(env, "Wrong number of arguments")
          .ThrowAsJavaScriptException();
    }

    if (!info[0].IsString() || !info[1].IsBuffer()) {
      Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    }

    // Packet stream identifier
    std::string id = info[0].As<Napi::String>().ToString();

    // Opus frame
    Napi::Buffer<const opus_byte> opus_buffer =
        info[1].As<Napi::Buffer<const opus_byte>>();
    auto opus_data_array = opus_buffer.Data();
    size_t opus_data_length = opus_buffer.Length();

    opus_frame frame(opus_data_array, opus_data_array + opus_data_length);

    // Submit to handler
    voice_manager->AddOpusFrame(id, frame);
  };

  // Callback with the detected command text
  void SendCommand(const std::string& id, const std::string& command_text) {
    this->node_callback->call([id, command_text](
                                  Napi::Env env,
                                  std::vector<napi_value>& args) {
      args = {Napi::String::New(env, id), Napi::String::New(env, command_text)};
    });
  }
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  // Setup logging
  SetupLogger();
  // Setup Detector class for NodeJS
  return Detector::Init(env, exports);
};

// n-api module entry point
NODE_API_MODULE(detector, InitAll)
