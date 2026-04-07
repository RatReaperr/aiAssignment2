#ifndef _WHISPER_TRANSCRIBER_HPP_
#define _WHISPER_TRANSCRIBER_HPP_

#include "whisper.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

class WhisperTranscriber
{
public:
  WhisperTranscriber(const std::string& model_path = SRC_DIR
                     "/models/ggml-base.en.bin")
  {
    whisper_context_params cparams = whisper_context_default_params();
    ctx_ = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (!ctx_)
      throw std::runtime_error("Failed to load Whisper model: " + model_path);
  }

  ~WhisperTranscriber()
  {
    whisper_free(ctx_);
  }

  WhisperTranscriber(const WhisperTranscriber&) = delete;
  WhisperTranscriber& operator=(const WhisperTranscriber&) = delete;

  std::string transcribe(const std::vector<float>& pcm)
  {
    if (pcm.empty())
      return {};

    whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress = false;
    wparams.print_timestamps = false;
    wparams.single_segment = true;
    wparams.n_threads = 4;

    if (whisper_full(ctx_, wparams, pcm.data(), static_cast<int>(pcm.size())) !=
        0)
      return {};

    std::string text;
    for (int i = 0; i < whisper_full_n_segments(ctx_); ++i)
      text += whisper_full_get_segment_text(ctx_, i);

    return text;
  }

private:
  whisper_context* ctx_{};
};

}

#endif // _WHISPER_TRANSCRIBER_HPP_
