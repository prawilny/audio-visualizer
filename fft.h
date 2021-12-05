#ifndef _AUDIO_VISUALIZER_FFT_H_
#define _AUDIO_VISUALIZER_FFT_H_

#include <complex>
#include <fftw3.h>
#include <vector>

std::vector<double> amplitudes_of_harmonics(std::vector<double> &wave_values);

#endif