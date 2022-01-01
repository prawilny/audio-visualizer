#ifndef _AUDIO_VISUALIZER_SHADER_UTILS_H_
#define _AUDIO_VISUALIZER_SHADER_UTILS_H_

#include "gl.h"
#include <string>

std::string file_read(const char *path);

GLuint create_shader(const char *filename, GLenum type);

GLuint create_program(const char *vertexfile, const char *fragmentfile);

GLint get_attrib(GLuint program, const char *name);

GLint get_uniform(GLuint program, const char *name);

#endif