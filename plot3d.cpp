#include "plot3d.h"
#include "SDL_scancode.h"
#include "fft.h"
#include "plot_utils.h"
#include "shader_utils.h"
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

static const char *VERTEX_SHADER = "plot3d.vertex.glsl";
static const char *FRAGMENT_SHADER = "plot3d.fragment.glsl";

static GLuint program;
static GLint attribute_coord3d;
static GLint uniform_vertex_transform;

static GLuint vbo[2];

static struct {
  const float r = 3;
  float inclination = 45.0;
  float azimuth = 0.0;
} eye;

static glm::vec3 eye_from_angles() {
  float azimuth_rads = glm::radians(eye.azimuth);
  float inclination_rads = glm::radians(eye.inclination);
  return glm::vec3(eye.r * cos(azimuth_rads) * sin(inclination_rads),
                   eye.r * cos(inclination_rads),
                   eye.r * sin(azimuth_rads) * sin(inclination_rads));
}

void plot3dInit() {
  program = create_program(VERTEX_SHADER, FRAGMENT_SHADER);
  if (program == 0) {
    throw std::runtime_error("couldnt't create plot3d program");
  }

  attribute_coord3d = get_attrib(program, "coord3d");
  uniform_vertex_transform = get_uniform(program, "vertex_transform");

  if (attribute_coord3d == -1 || uniform_vertex_transform == -1) {
    throw std::runtime_error("couldnt't get attribute");
  }
}

void plot3dDisplay(const std::vector<double> &fftLabels,
                   const std::deque<std::vector<double>> &fftValues,
                   SDL_AudioFormat fmt) {
  const size_t N = fftLabels.size();
  const size_t M = fftValues.size();
  const double labelSpan = span(fftLabels.data(), fftLabels.size());

  // Create two vertex buffer objects
  glGenBuffers(2, vbo);

  // Create an array for 101 * 101 vertices
  glm::vec3 vertices[N][M];
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      vertices[i][j].x = 2.0 * fftLabels[i] / labelSpan - 1.0;
      vertices[i][j].y = 2.0 * fftValues[j][i] / MAX_FFT_OUTPUT - 1.0;
      vertices[i][j].z = 2.0 * j / M - 1.0;
    }
  }

  // Tell OpenGL to copy our array to the buffer objects
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

  // Create an array of indices into the vertex array that traces both
  // horizontal and vertical lines
  const size_t num_indices = 2 * (M * (N - 1) + N * (M - 1));
  GLushort indices[num_indices];
  int idx = 0;

  for (int i = 0; i < M - 1; i++) {
    for (int j = 0; j < N; j++) {
      indices[idx++] = i * N + j;
      indices[idx++] = i * N + j + 1;
    }
  }

  for (int i = 0; i < N - 1; i++) {
    for (int j = 0; j < M; j++) {
      indices[idx++] = i * M + j;
      indices[idx++] = (i + 1) * M + j;
    }
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices,
               GL_STATIC_DRAW);

  glUseProgram(program);

  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = glm::lookAt(eye_from_angles(), glm::vec3(0.0, 0.0, 0.0),
                               glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f, 0.1f, 10.0f);
  glm::mat4 vertex_transform = projection * view * model;
  glUniformMatrix4fv(uniform_vertex_transform, 1, GL_FALSE,
                     glm::value_ptr(vertex_transform));

  /* Draw the grid using the indices to our vertices using our vertex buffer
   * objects */
  glEnableVertexAttribArray(attribute_coord3d);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glVertexAttribPointer(attribute_coord3d, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
  glDrawElements(GL_LINES, num_indices, GL_UNSIGNED_SHORT, 0);

  /* Stop using the vertex buffer object */
  glDisableVertexAttribArray(attribute_coord3d);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void plot3dHandleKeyEvent(const SDL_KeyboardEvent &event) {
  float delta = 3.0;
  switch (event.keysym.scancode) {
  case SDL_SCANCODE_LEFT:
    eye.azimuth -= delta;
    break;
  case SDL_SCANCODE_RIGHT:
    eye.azimuth += delta;
    break;
  case SDL_SCANCODE_UP:
    eye.inclination -= delta;
    if (eye.inclination <= 0.0) {
      eye.inclination += delta;
    }
    break;
  case SDL_SCANCODE_DOWN:
    eye.inclination += delta;
    if (eye.inclination >= 180.0) {
      eye.inclination -= delta;
    }
    break;
  default:
    break;
  }
}