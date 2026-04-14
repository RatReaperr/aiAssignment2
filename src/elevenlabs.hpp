#ifndef ELEVENLABS_HPP
#define ELEVENLABS_HPP

#include <string>
#include <string_view>

// Text-to-speech client for the ElevenLabs streaming API.
// Reads ELEVENLABS_API_KEY from the environment on construction.
// Call speak() to synthesise speech and play it through the default audio device.
class ElevenLabs
{
public:
  // model_id : ElevenLabs model, e.g. "eleven_flash_v2_5"
  // pcm_rate : PCM sample rate matching the output_format, e.g. 22050
  explicit ElevenLabs(std::string_view model_id = "eleven_flash_v2_5",
                      int pcm_rate = 22050);

  ElevenLabs(const ElevenLabs&)            = delete;
  ElevenLabs& operator=(const ElevenLabs&) = delete;

  // Synthesise text using voice_id and block until playback finishes.
  void speak(std::string_view text, std::string_view voice_id) const;

private:
  std::string api_key_;
  std::string model_id_;
  int         pcm_rate_;
};

#endif // ELEVENLABS_HPP
