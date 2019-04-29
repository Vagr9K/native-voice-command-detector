#pragma once
#include <opus/opus.h>
#include <functional>
#include <string>
#include <vector>

// Name alises for common types
using opus_byte = unsigned char;
using opus_frame = std::vector<opus_byte>;
using pcm_frame = int16_t;

using command_callback = std::function<void(std::string&, std::string&)>;
