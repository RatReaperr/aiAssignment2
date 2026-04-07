#include "elevenlabs.hpp"
#include "audio-player.hpp"
#include "http-streamer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
#include <string>

ElevenLabs::ElevenLabs(std::string_view model_id, int pcm_rate)
  : pcm_rate_{pcm_rate}, model_id_{model_id}
{
  const char* key = std::getenv("ELEVENLABS_API_KEY");
  if (!key)
    throw std::runtime_error("Set ELEVENLABS_API_KEY environment variable");
  api_key_ = key;
}

void ElevenLabs::speak(std::string_view text, std::string_view voice_id) const
{
  const std::string url = std::string{"https://api.elevenlabs.io/v1/text-to-speech/"}
                        + std::string{voice_id}
                        + "/stream?output_format=pcm_"
                        + std::to_string(pcm_rate_);

  const std::string body =
    nlohmann::json{{"text", text}, {"model_id", model_id_}}.dump();

  AudioPlayer  player{pcm_rate_};
  HttpStreamer  streamer;
  streamer.post(url, body,
                {{"xi-api-key", api_key_}, {"Content-Type", "application/json"}},
                [&player](const uint8_t* data, size_t bytes) {
                  player.receive(data, bytes);
                });
  player.drain();
}
