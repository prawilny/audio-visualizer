#ifndef _AUDIO_VISUALIZER_SPECTROGRAM_H_
#define _AUDIO_VISUALIZER_SPECTROGRAM_H_

#include <SDL_audio.h>
#include <cstddef>
#include <vector>

void spectrogramInit();

void spectrogramDisplay(double *fftLabels, double *fftValues, size_t fftN,
                        double *waveLabels, double *waveValues, size_t waveN,
                        SDL_AudioFormat fmt);

#endif