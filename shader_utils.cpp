#include "shader_utils.h"
#include "gl.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

std::string error_log(GLuint object) {
  GLint log_length = 0;
  if (glIsShader(object))
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else if (glIsProgram(object))
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else {
    throw std::runtime_error("not a shader nor a program");
  }

  char *log = (char *)malloc(log_length);

  if (glIsShader(object))
    glGetShaderInfoLog(object, log_length, NULL, log);
  else if (glIsProgram(object))
    glGetProgramInfoLog(object, log_length, NULL, log);

  // fprintf(stderr, "%s", log);
  std::string result(log, log_length);
  free(log);
  return result;
}

std::string file_read(const char *path) {
  std::ifstream ifs(path);
  if (ifs.fail()) {
    std::stringstream ss;
    ss << "file_read: " << path;
    throw std::runtime_error(ss.str());
  }
  return std::string((std::istreambuf_iterator<char>(ifs)),
                     (std::istreambuf_iterator<char>()));
}

GLuint create_shader(const char *filename, GLenum type) {
  const std::string source = file_read(filename);
  GLuint res = glCreateShader(type);
  const GLchar *sources[] = {"#version 120\n"
                             "#define lowp   \n"
                             "#define mediump\n"
                             "#define highp  \n",
                             source.data()};
  glShaderSource(res, 2, sources, NULL);

  glCompileShader(res);
  GLint compile_ok = GL_FALSE;
  glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
  if (compile_ok == GL_FALSE) {
    std::stringstream ss;
    ss << "shader compilation: " << filename << error_log(res);
    throw std::runtime_error(ss.str());
  }
  return res;
}

GLuint create_program(const char *vertexfile, const char *fragmentfile) {
  GLuint program = glCreateProgram();
  GLuint shader;

  if (vertexfile) {
    shader = create_shader(vertexfile, GL_VERTEX_SHADER);
    glAttachShader(program, shader);
  }

  if (fragmentfile) {
    shader = create_shader(fragmentfile, GL_FRAGMENT_SHADER);
    glAttachShader(program, shader);
  }

  glLinkProgram(program);
  GLint link_ok = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
  if (!link_ok) {
    std::stringstream ss;
    ss << "glLinkProgram: (" << vertexfile << ", " << fragmentfile << ")";
    throw std::runtime_error(ss.str());
  }
  return program;
}

GLint get_attrib(GLuint program, const char *name) {
  GLint attribute = glGetAttribLocation(program, name);
  if (attribute == -1) {
    std::stringstream ss;
    ss << "Could not bind attribute " << name;
    throw std::runtime_error(ss.str());
  }
  return attribute;
}

GLint get_uniform(GLuint program, const char *name) {
  GLint uniform = glGetUniformLocation(program, name);
  if (uniform == -1) {
    std::stringstream ss;
    ss << "Could not bind uniform " << name;
    throw std::runtime_error(ss.str());
  }
  return uniform;
}
