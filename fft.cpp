#include "fft.h"
#include <cmath>
#include <fftw3.h>
#include <memory>
#include <vector>
// TODO: Check single-precision FFT in case it bottlenecks the program.

#define RE_IDX 0
#define IM_IDX 1

// Here, the [0] element holds the real part and the [1] element holds the
// imaginary part. If you have a variable complex<double> *x, you can pass it
// directly to FFTW via reinterpret_cast<fftw_complex*>(x)

// TODO: the layout of data is: SAMPLE_LEFT || SAMPLE_RIGHT || SAMPLE_LEFT ||
// SAMPLE_RIGHT... What value should we use: sum/average? max?

std::unique_ptr<std::vector<double>>
amplitudes_of_harmonics(std::vector<double> &wave_values) {
  size_t n = wave_values.size();

  fftw_complex *in_out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * n);
  if (in_out == nullptr) {
    printf("Error: fftw_malloc()\n");
    exit(2);
  }

  fftw_plan plan =
      fftw_plan_dft_1d(n, in_out, in_out, FFTW_FORWARD, FFTW_ESTIMATE);

  for (size_t i = 0; i < n; i++) {
    in_out[i][RE_IDX] = wave_values[i];
    in_out[i][IM_IDX] = 0;
  }

  fftw_execute(plan);
  fftw_destroy_plan(plan);

  std::unique_ptr<std::vector<double>> result =
      std::make_unique<std::vector<double>>(n, 0);
  for (size_t i = 0; i < n; i++) {
    (*result)[i] = sqrt(in_out[i][RE_IDX] * in_out[i][RE_IDX] +
                        in_out[i][IM_IDX] * in_out[i][IM_IDX]);
  }

  fftw_free(in_out);

  return result;
}