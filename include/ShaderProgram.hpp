/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#pragma once

#include <string>
#include <filesystem>
#include <array>

#include <Neon.hpp>
#include <GL/gl3w.h>

#include "Debug.hpp"

namespace Akoylasar
{
  class ShaderProgram
  {
  public:
    ShaderProgram(const std::string& vs, const std::string& fs);
    ~ShaderProgram();
        
    GLint getUniformLocation(const char* const uniformName) const;
    GLuint getUniformBlockIndex(const char* const uniformBlockName) const;
    
    void setFloatUniform(const GLuint location, float value) const;
    void setVec2fUniform(const GLuint location, const Neon::Vec2f& vec) const;
    void setVec3fUniform(const GLuint location, const Neon::Vec3f& vec) const;
    void setIntUniform(const GLuint location, int value) const;
    template<int N> void setVec3fArrayUniform(const GLuint location, const std::array<Neon::Vec3f, N>& value) const
    {
      CHECK_GL_ERROR(glUniform3fv(location, N, reinterpret_cast<const float*>(value.data())));
    }
    void setMat4fUniform(const GLuint location, const Neon::Mat4f& mat) const;
    void setUniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    void use() const;

  private:
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

  private:
    static bool createShader(GLuint& shader, const std::string& source, GLenum type, std::string& error);

  private:
    GLuint mProgramHandle;
  };
}
