#include "audio-player.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>

AudioPlayer::AudioPlayer(int freq) : freq_{freq}
{
  // Signed 16-bit little-endian, mono at the requested sample rate.
  // Starts paused — resumed once the pre-buffer threshold is reached.
  SDL_AudioSpec spec{};
  spec.format   = SDL_AUDIO_S16;
  spec.channels = 1;
  spec.freq     = freq_;
  stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                      &spec, nullptr, nullptr);
  if (!stream_)
    throw std::runtime_error(SDL_GetError());
  SDL_SetAudioStreamGetCallback(stream_, on_need_data, this);
}

AudioPlayer::~AudioPlayer()
{
  SDL_DestroyAudioStream(stream_);
}

void AudioPlayer::receive(const uint8_t* data, size_t bytes)
{
  int buffered;
  {
    std::lock_guard lk(mutex_);
    pending_.insert(pending_.end(), data, data + bytes);
    buffered = (int)pending_.size();
  }
  if (!started_ && buffered >= freq_ * 2 / 2) // ~0.5 s of S16 mono
  {
    SDL_ResumeAudioStreamDevice(stream_);
    started_ = true;
  }
}

void AudioPlayer::drain()
{
  if (!started_)
  {
    SDL_ResumeAudioStreamDevice(stream_);
    started_ = true;
  }
  for (;;)
  {
    {
      std::lock_guard lk(mutex_);
      if (pending_.empty())
        break;
    }
    SDL_Delay(50);
  }
  SDL_Delay(100);
  SDL_FlushAudioStream(stream_);
  while (SDL_GetAudioStreamQueued(stream_) > 0)
    SDL_Delay(50);
  SDL_Delay(300);
}

void AudioPlayer::on_need_data(void* userdata, SDL_AudioStream* stream,
                               int need, int)
{
  auto* self = static_cast<AudioPlayer*>(userdata);
  std::lock_guard lk(self->mutex_);
  if (self->pending_.empty())
    return;
  const int give = std::min((int)self->pending_.size(), need);
  SDL_PutAudioStreamData(stream, self->pending_.data(), give);
  self->pending_.erase(self->pending_.begin(), self->pending_.begin() + give);
}
