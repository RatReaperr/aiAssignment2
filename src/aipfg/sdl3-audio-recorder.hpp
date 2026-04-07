#ifndef _SDL3_AUDIO_RECORDER_HPP_
#define _SDL3_AUDIO_RECORDER_HPP_

#include <SDL3/SDL.h>
#include <stdexcept>
#include <vector>

namespace
{

class SDL3_AudioRecorder
{
public:
  SDL3_AudioRecorder(int freq = 16000, int channels = 1)
  {
    SDL_AudioSpec spec{};
    spec.freq = freq;
    spec.format = SDL_AUDIO_F32;
    spec.channels = channels;

    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING,
                                        &spec, audio_callback, &buffer_);

    if (!stream_)
      throw std::runtime_error("Failed to open audio device");
  }

  ~SDL3_AudioRecorder()
  {
    SDL_DestroyAudioStream(stream_);
  }

  void resume()
  {
    SDL_ResumeAudioStreamDevice(stream_);
  }

  void pause()
  {
    SDL_PauseAudioStreamDevice(stream_);
  }

  void clear()
  {
    buffer_.clear();
  }

  const std::vector<float>& buffer() const
  {
    return buffer_;
  }

private:
  static void audio_callback(void* userdata, SDL_AudioStream* stream,
                             int additional_amount, int /*total_amount*/)
  {
    auto&              buf = *static_cast<std::vector<float>*>(userdata);
    int                samples = additional_amount / sizeof(float);
    std::vector<float> temp(samples);
    int bytes = SDL_GetAudioStreamData(stream, temp.data(), additional_amount);
    int got = bytes / sizeof(float);
    buf.insert(buf.end(), temp.begin(), temp.begin() + got);
  }

  SDL_AudioStream*   stream_{};
  std::vector<float> buffer_;
};

}

#endif // _SDL3_AUDIO_RECORDER_HPP_
