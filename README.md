# Native Voice Command Detector

Native voice command detector is a multithreaded N-API based native NodeJS module that can be used to detect voice commands.

The entire audio processing queue is handled by this module to prevent N-API overhead.

Chunks of processing are distributed between worker threads and utilize parallelization for maximum performance.

The amount of created worker threads is `logical_cpu_cores + 1`, where 1 thread schedules the work.

The project as of right now supports receiving RAW OPUS frames on Linux X86_64 platforms and invokes a callback with the text of the command whenever it's detected.
Multiple audio streams are supported via unique IDs.

The module relies on [Porcupine](https://github.com/Picovoice/Porcupine) for hotword detection and [Google Speech To Text API](https://cloud.google.com/speech-to-text/) for speech recognition.

This project was primarily created for the [Discord VoiceBot](https://github.com/Vagr9K/VoiceBot), since a NodeJS only solution wouldn't be able to provide the desired performance.

## Building

- Install the external dependencies by relying on the system package manager:

  * libopus
  * libopusenc
  * libcurl
  * libssl
  * libcrypto

- Install the internal dependencies by running `git submodule update --init --recursive`
- Run `yarn` or `npm install` to setup the build environment
- Run `yarn build` or `npm run build` to build the module
- The module will be located in `build/Release/detector.node`
- NOTE: This project is in an alpha state and doesn't have prebuilt binaries ready. TO be able to `require` this module, rely on `yarn link` or `npm link`.

## Usage

First, initialize a `Detector` instance with the correct configuration:

```js
const Detector = require("native-voice-command-detector");

const voiceCommandDetector = new Detector(
    pv_model_path,
    pv_keyword_path,
    pv_sensitivity,
    gcloud_speech_to_text_api_key,
    max_voice_buffer_ttl,
    max_command_length,
    max_command_silence_length_ms,
    callback
    );
```

`pv_model_path` and `pv_keyword_path` specify the hotword to detect. Consult [Porcupine's documentation on how to generate these files](https://github.com/Picovoice/Porcupine/tree/master/tools/optimizer).

`pv_sensitivity` configures the sensitivity and should be a float between `0` (lowest sensitivity) and `1` (highest sensitivity).

`gcloud_speech_to_text_api_key` is the API key that will be used for GCloud based operations. Consult the [GCloud API Key documentation](https://cloud.google.com/docs/authentication/api-keys) for details.

`max_voice_buffer_ttl` is the amount of time in milliseconds a voice data buffer can spend being filled, without being processed by a worker thread.

`max_command_length` specifies the maximum length of a voice command in milliseconds, after which the speech recognition will begin.

`max_command_silence_length_ms` specifies the length of silence (no input) in milliseconds after which the command sequence will be treated as complete and the speech recognition will begin.

`callback` will be called upon a keyword being detected:

```js
  const callback = (id, command) => {
      // ID is a string containing the audio source identification
      // command is the detected command text
      console.log(id, command)
  };
```

After the instance is initialized, submit audio data via:

```js
commandDetector.addOpusFrame(id, buf);
```

Where `buf` is a `Buffer` containing the binary data of the OPUS frame.

## TypeScript

TypeScript definitions are available out of the box in `lib/index.d.ts`.

## Copyright

Copyright (c) 2019 Ruben Harutyunyan
