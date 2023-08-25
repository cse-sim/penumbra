/* Copyright (c) 2017 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// Standard
#include <iostream>

// Penumbra
#include <penumbra/logging.h>
#include "shader.h"

namespace Pumbra {
GLShader::GLShader(GLenum type, const char *source,
                   const std::shared_ptr<Courierr::Courierr> &logger_in)
    : logger(logger_in) {
  GLint shader_ok;
  GLsizei log_length;
  char info_log[8192];

  shader = glCreateShader(type);
  if (shader != 0) {
    glShaderSource(shader, 1, (const GLchar **)&source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if (shader_ok != GL_TRUE) {
      glGetShaderInfoLog(shader, 8192, &log_length, info_log);
      glDeleteShader(shader);
      shader = 0;
      std::string shaderTypeStr = (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex";
      logger->info(fmt::format("OpenGL {} shader: {}", shaderTypeStr, info_log));
      throw PenumbraException(fmt::format("Unable to compile {} shader.", shaderTypeStr), *logger);
    }
  }
}

GLShader::~GLShader() {}

GLuint GLShader::getInt() {
  return shader;
}

} // namespace Pumbra
