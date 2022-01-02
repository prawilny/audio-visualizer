#include "spectrogram.h"
#include "SDL_audio.h"
#include "fft.h"
#include "gl.h"
#include "shader_utils.h"
#include <SDL_opengl.h>
#include <limits>
#include <stdexcept>
#include <vector>

// TODO: return to GLdouble
struct point {
  GLfloat x;
  GLfloat y;
};

static const char *FFT_VERTEX_SHADER = "fft.vertex.glsl";
static const char *WAVE_VERTEX_SHADER = "wave.vertex.glsl";
static const char *FFT_FRAGMENT_SHADER = "fft.fragment.glsl";
static const char *WAVE_FRAGMENT_SHADER = "wave.fragment.glsl";

GLuint fft_program;
GLuint wave_program;
GLint fft_attr_coord2d;
GLint wave_attr_coord2d;

void spectrogramInit() {
  fft_program = create_program(FFT_VERTEX_SHADER, FFT_FRAGMENT_SHADER);
  fft_attr_coord2d = get_attrib(fft_program, "coord2d");

  wave_program = create_program(WAVE_VERTEX_SHADER, WAVE_FRAGMENT_SHADER);
  wave_attr_coord2d = get_attrib(wave_program, "coord2d");
}

static void display(std::vector<point> &graph, GLuint program,
                    GLint attr_coord2d) {
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(point) * graph.size(), graph.data(),
               GL_DYNAMIC_DRAW);

  glUseProgram(program);

  glEnableVertexAttribArray(attr_coord2d);
  glVertexAttribPointer(attr_coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glLineWidth(2.5);
  glDrawArrays(GL_LINE_STRIP, 0, graph.size());
}

static double span(double *data, size_t n) {
  double max = 0, min = 0;
  for (size_t i = 0; i < n; i++) {
    max = std::max(max, data[i]);
    min = std::min(min, data[i]);
  }
  return max - min;
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

static double scaleY(double y, SDL_AudioFormat format) {
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
  display(waveGraphData, wave_program, wave_attr_coord2d);
}