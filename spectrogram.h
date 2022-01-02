#ifndef _AUDIO_VISUALIZER_SPECTROGRAM_H_
#define _AUDIO_VISUALIZER_SPECTROGRAM_H_

#include <cstddef>
#include <vector>

void spectrogramInit();

void spectrogramDisplay(double *labels, double *values, size_t n);

#endif