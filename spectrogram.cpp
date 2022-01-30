#include "spectrogram.h"
#include "SDL_audio.h"
#include "fft.h"
#include "gl.h"
#include "global.h"
#include "plot_utils.h"
#include "shader_utils.h"
#include <SDL_opengl.h>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>

struct point {
  GLfloat x;
  GLfloat y;
};

static const char *FFT_VERTEX_SHADER = "fft.vertex.glsl";
static const char *WAVE_VERTEX_SHADER = "wave.vertex.glsl";
static const char *FFT_FRAGMENT_SHADER = "fft.fragment.glsl";
static const char *WAVE_FRAGMENT_SHADER = "wave.fragment.glsl";

static GLuint fft_program;
static GLuint wave_program;
static GLint fft_attr_coord2d;
static GLint wave_attr_coord2d;
static GLuint vbo;

void spectrogramInit() {
  fft_program = create_program(FFT_VERTEX_SHADER, FFT_FRAGMENT_SHADER);
  fft_attr_coord2d = get_attrib(fft_program, "coord2d");

  wave_program = create_program(WAVE_VERTEX_SHADER, WAVE_FRAGMENT_SHADER);
  wave_attr_coord2d = get_attrib(wave_program, "coord2d");

  glGenBuffers(1, &vbo);
}

static void display(std::vector<point> &graph, GLuint program,
                    GLint attr_coord2d) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(point) * graph.size(), graph.data(),
               GL_DYNAMIC_DRAW);

  glUseProgram(program);

  glEnableVertexAttribArray(attr_coord2d);
  glVertexAttribPointer(attr_coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glLineWidth(2.5);
  glDrawArrays(GL_LINE_STRIP, 0, graph.size());
}

static std::vector<point> fftGraph(double *labels, double *values, size_t n) {
  std::vector<point> graph(n);
  double labelSpan = span(labels, n);
  for (size_t i = 0; i < n; i++) {
    graph[i].x = 2 * (labels[i] / labelSpan - 0.5);
    graph[i].y = values[i] / MAX_FFT_OUTPUT;
  }
  return graph;
}

static std::vector<point> waveGraph(double *labels, double *values, size_t n,
                                    SDL_AudioFormat format) {
  std::vector<point> graph(n);
  double labelSpan = span(labels, n);
  for (size_t i = 0; i < n; i++) {
    graph[i].x = 2 * (labels[i] / labelSpan - 0.5);

    double scaledY = scaleY(values[i], format);

    graph[i].y = scaledY / 4 - 0.5;
  }
  return graph;
}

void spectrogramDisplay(double *fftLabels, double *fftValues, size_t fftN,
                        double *waveLabels, double *waveValues, size_t waveN,
                        SDL_AudioFormat format) {
  std::vector<point> fftData = fftGraph(fftLabels, fftValues, fftN);
  display(fftData, fft_program, fft_attr_coord2d);

  std::vector<point> waveGraphData =
      waveGraph(waveLabels, waveValues, waveN, format);
  big_lock.unlock();
  display(waveGraphData, wave_program, wave_attr_coord2d);
}