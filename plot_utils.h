#ifndef _AUDIO_VISUALIZER_PLOT_UTILS_H_
#define _AUDIO_VISUALIZER_PLOT_UTILS_H_

#include "SDL_audio.h"

double scaleY(double y, SDL_AudioFormat format);

double span(const double *data, size_t n);

#endif
