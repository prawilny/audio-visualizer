#include "spectrogram.h"
#include "gl.h"
#include <SDL_opengl.h>
#include <vector>

struct point {
  GLfloat x;
  GLfloat y;
};

void displaySpectrogram(double *labels, double *values, size_t n) {
  std::vector<point> graph(n);
  for (int i = 0; i < n; i++) {
    graph[i].x = labels[i];
    graph[i].y = values[i];
  }

  GLuint vbo;
  glGenBuffers(1, &vbo);
}