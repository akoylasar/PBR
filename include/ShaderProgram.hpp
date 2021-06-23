#pragma once

#include <string>
#include <filesystem>

#include <Neon.hpp>
#include <GL/gl3w.h>

namespace Akoylasar
{
  class ShaderProgram
  {
  public:
    ShaderProgram(const std::string& vs, const std::string& fs);
    ~ShaderProgram();
        
    GLint getUniformLocation(const std::string& uniformName) const;

    void setFloatUniform(const GLuint location, float value) const;
    void setVec2fUniform(const GLuint location, Neon::Vec2<float>& vec) const;
    void setVec3fUniform(const GLuint location, Neon::Vec3<float>& vec) const;
    void setMat4fUniform(const GLuint location, const Neon::Mat4<float>& mat) const;
    
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
