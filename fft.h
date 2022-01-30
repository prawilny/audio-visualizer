#ifndef _AUDIO_VISUALIZER_FFT_H_
#define _AUDIO_VISUALIZER_FFT_H_

#include <complex>
#include <fftw3.h>
#include <memory>
#include <vector>

const uint64_t MAX_FFT_OUTPUT = 11000000; // Value for 50 FPS (50Hz FFT resolution).

std::vector<double> amplitudes_of_harmonics(std::vector<double> &wave_values);

#endif