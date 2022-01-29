#include "plot3d.h"
#include "SDL_scancode.h"
#include "fft.h"
#include "global.h"
#include "plot_utils.h"
#include "shader_utils.h"
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <mutex>
#include <stdexcept>

static const char *VERTEX_SHADER = "plot3d.vertex.glsl";
static const char *FRAGMENT_SHADER = "plot3d.fragment.glsl";

static const float SQRT_MAX_FFT_OUTPUT = sqrt(MAX_FFT_OUTPUT);

static GLuint program;
static GLint attribute_coord3d;
static GLint uniform_vertex_transform;
static GLuint vbo;

static const float DRAW_DISTANCE = 10.0;

static struct {
  float r = 3;
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

// static int i = 0;

void plot3dDisplay(const std::vector<double> &fftLabels,
                   const std::deque<std::vector<double>> &fftValues,
                   SDL_AudioFormat fmt) {
  const size_t N = fftLabels.size();
  for (auto fftValueVector : fftValues) {
    if (fftValueVector.size() != N) {
      throw std::runtime_error("sizes should be equal!");
    }
  }
  const size_t M = fftValues.size();
  const double labelSpan = span(fftLabels.data(), fftLabels.size());

  glGenBuffers(1, &vbo);

  // std::vector<float> ys;

  glm::vec3 vertices[N][M];
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      vertices[i][j].x = 2.0 * fftLabels[i] / labelSpan - 1.0;
      vertices[i][j].y =
          2.0 * sqrt(fftValues[j][i]) / SQRT_MAX_FFT_OUTPUT - 1.0;
      vertices[i][j].z = 2.0 * j / M - 1.0;

      // if (j == 0) {
      //   ys.push_back(vertices[i][j].y);
      // }
    }
  }
  big_lock.unlock();

  // if (i % 1000 == 0) {
  //   std::cerr << "[";
  //   for (auto y : ys) {
  //     std::cout << (y + 1.0) / 2 << ",";
  //   }
  //   std::cerr << "]" << std::endl;
  // }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

  glUseProgram(program);

  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 view = glm::lookAt(eye_from_angles(), glm::vec3(0.0, 0.0, 0.0),
                               glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f, 0.1f, DRAW_DISTANCE);
  glm::mat4 vertex_transform = projection * view * model;
  glUniformMatrix4fv(uniform_vertex_transform, 1, GL_FALSE,
                     glm::value_ptr(vertex_transform));

  glEnableVertexAttribArray(attribute_coord3d);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(attribute_coord3d, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glPointSize(4);
  glDrawArrays(GL_POINTS, vbo, M * N);
}

void plot3dHandleKeyEvent(const SDL_KeyboardEvent &event) {
  float deltaDegs = 3.0;
  float deltaDistance = 0.05;
  switch (event.keysym.scancode) {
  case SDL_SCANCODE_LEFT:
    eye.azimuth -= deltaDegs;
    break;
  case SDL_SCANCODE_RIGHT:
    eye.azimuth += deltaDegs;
    break;
  case SDL_SCANCODE_UP:
    eye.inclination -= deltaDegs;
    if (eye.inclination <= 0.0) {
      eye.inclination += deltaDegs;
    }
    break;
  case SDL_SCANCODE_DOWN:
    eye.inclination += deltaDegs;
    if (eye.inclination >= 180.0) {
      eye.inclination -= deltaDegs;
    }
    break;
  case SDL_SCANCODE_EQUALS:
    if (eye.r > deltaDistance) {
      eye.r -= deltaDistance;
    }
    break;
  case SDL_SCANCODE_MINUS:
    eye.r += deltaDistance;
    if (eye.r > DRAW_DISTANCE - 2.0) {
      eye.r -= deltaDistance;
    }
    break;
  default:
    break;
  }
}