#include "plot_utils.h"
#include <stdexcept>

double scaleY(double y, SDL_AudioFormat format) {
  switch (format) {
  case AUDIO_S8:
    return y / std::numeric_limits<int8_t>::max();
  case AUDIO_S16:
    return y / std::numeric_limits<int16_t>::max();
  case AUDIO_S32:
    return y / std::numeric_limits<int32_t>::max();
  case AUDIO_U8:
    return y / (std::numeric_limits<uint8_t>::max() / 2) - 1;
  case AUDIO_U16:
    return y / (std::numeric_limits<uint16_t>::max() / 2) - 1;
  default:
    throw std::runtime_error("scaleY: unsupported audio format");
  }
}

double span(const double *data, size_t n) {
  double max = 0, min = 0;
  for (size_t i = 0; i < n; i++) {
    max = std::max(max, data[i]);
    min = std::min(min, data[i]);
  }
  return max - min;
}
