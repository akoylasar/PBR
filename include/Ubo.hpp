/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#include <memory>

#include <GL/gl3w.h>

#include "Debug.hpp"

class Ubo
{
public:
  static std::unique_ptr<Ubo> createUbo(unsigned int sizeInBytes, GLuint uniformBlockBinding)
  {
    auto ubo = std::make_unique<Ubo>();
    CHECK_GL_ERROR(glGenBuffers(1, &ubo->handle));
    CHECK_GL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, ubo->handle));
    CHECK_GL_ERROR(glBufferData(GL_UNIFORM_BUFFER, sizeInBytes, nullptr, GL_STATIC_DRAW));
    CHECK_GL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    // @todo: Separate.
    CHECK_GL_ERROR(glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockBinding, ubo->handle, 0, sizeInBytes));
    return ubo;
  }

  static void updateUbo(const Ubo& ubo, 
                        GLintptr offsetInBytes,
                        GLsizeiptr sizeInBytes,
                        const GLvoid* data)
  {
    CHECK_GL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, ubo.handle));
    CHECK_GL_ERROR(glBufferSubData(GL_UNIFORM_BUFFER, offsetInBytes, sizeInBytes, data));
    CHECK_GL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
  }

  static void releaseUbo(const Ubo& ubo)
  {
    CHECK_GL_ERROR(glDeleteBuffers(1, &ubo.handle));
  }

private:
  GLuint handle;
};
