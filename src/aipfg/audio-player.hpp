#ifndef AUDIO_PLAYER_HPP
#define AUDIO_PLAYER_HPP

#include <cstdint>
#include <mutex>
#include <vector>

struct SDL_AudioStream;

// Buffers streaming PCM bytes from the network and drives SDL audio playback.
// on_need_data (SDL's pull callback) is the only place data enters the stream,
// keeping SDL's internal resampler in a consistent state with no chunk-boundary
// surprises.
class AudioPlayer
{
public:
  explicit AudioPlayer(int freq = 22050);
  ~AudioPlayer();

  AudioPlayer(const AudioPlayer&)            = delete;
  AudioPlayer& operator=(const AudioPlayer&) = delete;

  // Called from the network thread as each PCM chunk arrives.
  void receive(const uint8_t* data, size_t bytes);

  // Ensures playback has started, then blocks until the stream is silent.
  void drain();

private:
  // SDL's pull callback: runs on the audio thread, feeds the stream on demand.
  static void on_need_data(void* userdata, SDL_AudioStream* stream,
                           int need, int available);

  int freq_; // Sample rate used to compute the pre-buffer threshold.

  SDL_AudioStream*     stream_  = nullptr;
  std::vector<uint8_t> pending_;
  std::mutex           mutex_;
  bool                 started_ = false;
};

#endif // AUDIO_PLAYER_HPP
