#ifndef _AUDIO_VISUALIZER_CONVERTER_H_
#define _AUDIO_VISUALIZER_CONVERTER_H_

#include <SDL_audio.h>
#include <cstdint>
#include <vector>

struct PCM_data {
  std::vector<uint8_t> bytes;
  SDL_AudioFormat format = 0;
  int channels = 0;
  long rate = 0;
  size_t processed_bytes;
};

PCM_data from_mp3(const char *filename);

#endif // _AUDIO_VISUALIZER_CONVERTER_H_
