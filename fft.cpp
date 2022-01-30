#include "fft.h"
#include <cmath>
#include <fftw3.h>
#include <memory>
#include <vector>

#define RE_IDX 0
#define IM_IDX 1

std::vector<double> amplitudes_of_harmonics(std::vector<double> &wave_values) {
  size_t n = wave_values.size();

  fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * n);
  if (out == nullptr) {
    printf("Error: fftw_malloc()\n");
    exit(2);
  }

  fftw_plan plan =
      fftw_plan_dft_r2c_1d(n, wave_values.data(), out, FFTW_ESTIMATE);

  fftw_execute(plan);
  fftw_destroy_plan(plan);

  std::vector<double> result = std::vector<double>(n / 2, 0);
  for (size_t i = 0; i < n / 2; i++) {
    result[i] =
        sqrt(out[i][RE_IDX] * out[i][RE_IDX] + out[i][IM_IDX] * out[i][IM_IDX]);
  }

  fftw_free(out);

  return result;
}