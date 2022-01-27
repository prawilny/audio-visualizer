#include "plot3d.h"
#include "shader_utils.h"
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

static const char *VERTEX_SHADER = "plot3d.vertex.glsl";
static const char *FRAGMENT_SHADER = "plot3d.fragment.glsl";

static GLuint program;
static GLint attribute_coord2d;
static GLint uniform_vertex_transform;

static GLuint vbo[2];

void plot3dInit() {
  program = create_program(VERTEX_SHADER, FRAGMENT_SHADER);
  if (program == 0) {
    throw std::runtime_error("couldnt't create plot3d program");
  }

  attribute_coord2d = get_attrib(program, "coord2d");
  uniform_vertex_transform = get_uniform(program, "vertex_transform");

  if (attribute_coord2d == -1 || uniform_vertex_transform == -1) {
    throw std::runtime_error("couldnt't get attribute");
  }
}

void plot3dDisplay(const std::vector<double> &fftLabels,
                   const std::deque<std::vector<double>> &fftValues,
                   SDL_AudioFormat fmt) {
  // Create two vertex buffer objects
  glGenBuffers(2, vbo);

  // Create an array for 101 * 101 vertices
  glm::vec2 vertices[101][101];

  for (int i = 0; i < 101; i++) {
    for (int j = 0; j < 101; j++) {
      vertices[i][j].x = (j - 50) / 50.0;
      vertices[i][j].y = (i - 50) / 50.0;
    }
  }

  // Tell OpenGL to copy our array to the buffer objects
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

  // Create an array of indices into the vertex array that traces both
  // horizontal and vertical lines
  GLushort indices[100 * 101 * 4];
  int i = 0;

  for (int y = 0; y < 101; y++) {
    for (int x = 0; x < 100; x++) {
      indices[i++] = y * 101 + x;
      indices[i++] = y * 101 + x + 1;
    }
  }

  for (int x = 0; x < 101; x++) {
    for (int y = 0; y < 100; y++) {
      indices[i++] = y * 101 + x;
      indices[i++] = (y + 1) * 101 + x;
    }
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices,
               GL_STATIC_DRAW);

  glUseProgram(program);

  glm::mat4 model;
  model = glm::mat4(1.0f);

  glm::mat4 view =
      glm::lookAt(glm::vec3(0.0, -2.0, 2.0), glm::vec3(0.0, 0.0, 0.0),
                  glm::vec3(0.0, 0.0, 1.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f * 640 / 480, 0.1f, 10.0f);

  glm::mat4 vertex_transform = projection * view * model;

  glUniformMatrix4fv(uniform_vertex_transform, 1, GL_FALSE,
                     glm::value_ptr(vertex_transform));

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  /* Draw the grid using the indices to our vertices using our vertex buffer
   * objects */
  glEnableVertexAttribArray(attribute_coord2d);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glVertexAttribPointer(attribute_coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
  glDrawElements(GL_LINES, 100 * 101 * 4, GL_UNSIGNED_SHORT, 0);

  /* Stop using the vertex buffer object */
  glDisableVertexAttribArray(attribute_coord2d);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
