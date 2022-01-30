#ifndef _AUDIO_VISUALIZER_PLOT3D_H_
#define _AUDIO_VISUALIZER_PLOT3D_H_

#include "SDL_audio.h"
#include "SDL_events.h"
#include <deque>
#include <vector>

void plot3dInit();

void plot3dDisplay(const std::vector<double> &fftLabels,
                   const std::deque<std::vector<double>> &fftValues,
                   SDL_AudioFormat fmt);

void plot3dHandleKeyEvent();

#endif