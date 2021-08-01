/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#include "Debug.hpp"

#include <GL/gl3w.h>

namespace Akoylasar
{
  void checkGLError(const char* cmdName, const char* file, int line)
  {
    if (GLenum errorCode = glGetError())
    {
      std::string error;
      switch (errorCode)
      {
        case GL_INVALID_ENUM:
          error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:
          error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:
          error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:
          error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:
          error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:
          error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
          error = "INVALID_FRAMEBUFFER_OPERATION"; break;
      }
      std::cerr << cmdName << " had error " << error << " @ " << file << " (" << line << ")" << std::endl;
    }
  }
  
  void clearGLErrors()
  {
    while (GLenum errorCode = glGetError()) {}
  }
}
