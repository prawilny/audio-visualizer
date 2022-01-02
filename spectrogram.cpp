#include "spectrogram.h"
#include "fft.h"
#include "gl.h"
#include "shader_utils.h"
#include <SDL_opengl.h>
#include <vector>

// TODO: return to GLdouble
struct point {
  GLfloat x;
  GLfloat y;
};

static const char *VERTEX_SHADER = "spectrogram.vertex.glsl";
static const char *FRAGMENT_SHADER = "spectrogram.fragment.glsl";

GLuint program;
GLint attribute_coord2d;

void spectrogramInit() {
  program = create_program(VERTEX_SHADER, FRAGMENT_SHADER);
  attribute_coord2d = get_attrib(program, "coord2d");
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
}

void spectrogramDisplay(double *labels, double *values, size_t n) {
  std::vector<point> graph(n);

  double maxLabel = 0, minLabel = 0;
  for (size_t i = 0; i < n; i++) {
    maxLabel = std::max(maxLabel, labels[i]);
    minLabel = std::min(minLabel, labels[i]);
  }
  double labelSpan = maxLabel - minLabel;

  for (size_t i = 0; i < n; i++) {
    graph[i].x = 2 * (labels[i] / labelSpan - 0.5);
    graph[i].y = values[i] / MAX_FFT_OUTPUT;
  }

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(point) * n, graph.data(),
               GL_DYNAMIC_DRAW);

  glUseProgram(program);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glEnableVertexAttribArray(attribute_coord2d);
  glVertexAttribPointer(attribute_coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawArrays(GL_LINE_STRIP, 0, n);
}