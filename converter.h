#ifndef _AUDIO_VISUALIZER_CONVERTER_H_
#define _AUDIO_VISUALIZER_CONVERTER_H_

#include <cstdint>
#include <vector>

struct PCM_data {
  // Might be padded with zeroes.
  std::vector<uint8_t> bytes;
  int channels = 0;
  int encoding = 0;
  long rate = 0;
};

PCM_data from_mp3(const char *filename);

#endif // _AUDIO_VISUALIZER_CONVERTER_H_
