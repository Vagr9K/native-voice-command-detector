export default class Detector {
  constructor(
    pv_model_path: string,
    pv_keyword_path: string,
    pv_sensitivity: number,
    gcloud_speech_to_text_api_key: string,
    max_voice_buffer_ttl: number,
    max_command_length: number,
    max_command_silence_length_ms: number,
    callback: (id: string, command: string) => void
  );
  addOpusFrame: (id: string, opusFrameBuffer: Buffer) => void;
}
