#include "ShaderProgram.hpp"

namespace Akoylasar
{
  ShaderProgram::ShaderProgram(const std::string& vsSource, const std::string& fsSource)
  {
    std::string vsError; GLuint vs;
    if (!createShader(vs, vsSource, GL_VERTEX_SHADER, vsError))
      DEBUG_ASSERT_MSG(false, vsError);
    
    std::string fsError; GLuint fs;
    if (!createShader(fs, fsSource, GL_FRAGMENT_SHADER, fsError))
      DEBUG_ASSERT_MSG(false, fsError);

    mProgramHandle = CHECK_GL_ERROR(glCreateProgram());
    
    CHECK_GL_ERROR(glAttachShader(mProgramHandle, vs));
    CHECK_GL_ERROR(glAttachShader(mProgramHandle, fs));
    CHECK_GL_ERROR(glLinkProgram(mProgramHandle));
    
    GLint linked;
    CHECK_GL_ERROR(glGetProgramiv(mProgramHandle, GL_LINK_STATUS, &linked));
    if (linked != GL_TRUE)
    {
      GLchar message[1024];
      CHECK_GL_ERROR(glGetProgramInfoLog(mProgramHandle, 1024, nullptr, message));
      DEBUG_ASSERT_MSG(false, message);
    }
    
    CHECK_GL_ERROR(glDeleteShader(vs));
    CHECK_GL_ERROR(glDeleteShader(fs));
  }

  ShaderProgram::~ShaderProgram()
  {
    CHECK_GL_ERROR(glDeleteProgram(mProgramHandle));
  }
  
  GLint ShaderProgram::getUniformLocation(const std::string& uniformName) const
  {
    return CHECK_GL_ERROR(glGetUniformLocation(mProgramHandle, uniformName.c_str()));
  }
  
  // @todo:
  // In OpenGL 4.2 and onward we could use this
  // layout(std140, binding = 0) uniform Block { ... };
  GLuint ShaderProgram::getUniformBlockIndex(const std::string& uniformBlockName) const
  {
    return CHECK_GL_ERROR(glGetUniformBlockIndex(mProgramHandle, uniformBlockName.c_str()));
  }

  void ShaderProgram::setFloatUniform(const GLuint location, float value) const
  {
    CHECK_GL_ERROR(glUniform1f(location, value));
  }

  void ShaderProgram::setVec2fUniform(const GLuint location, const Neon::Vec2f& vec) const
  {
    CHECK_GL_ERROR(glUniform2f(location, vec.x, vec.y));
  }

  void ShaderProgram::setVec3fUniform(const GLuint location, const Neon::Vec3f& vec) const
  {
    CHECK_GL_ERROR(glUniform3f(location, vec.x, vec.y, vec.z));
  }

  void ShaderProgram::setMat4fUniform(const GLuint location, const Neon::Mat4f& mat) const
  {
    CHECK_GL_ERROR(glUniformMatrix4fv(location, 1, GL_FALSE, mat.data()));
  }
  
  void ShaderProgram::setIntUniform(const GLuint location, int value)
  {
    CHECK_GL_ERROR(glUniform1i(location, value));
  }
  
  void ShaderProgram::use() const
  {
    CHECK_GL_ERROR(glUseProgram(mProgramHandle));
  }

  bool ShaderProgram::createShader(GLuint& shader, const std::string& source, GLenum type, std::string& error)
  {
    shader = CHECK_GL_ERROR(glCreateShader(type));
    if (shader == 0) return false;
    
    const GLchar* sourceAddress = &source[0];
    CHECK_GL_ERROR(glShaderSource(shader, 1, &sourceAddress, nullptr));
    CHECK_GL_ERROR(glCompileShader(shader));
    
    GLint compiled;
    CHECK_GL_ERROR(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
    if (compiled != GL_TRUE)
    {
      GLsizei logLength = 0;
      CHECK_GL_ERROR(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength));
      error.resize(logLength);
      CHECK_GL_ERROR(glGetShaderInfoLog(shader, 1024, &logLength, &error[0]));
      return false;
    }
    return true;
  }
  
  void ShaderProgram::setUniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBinding)
  {
    CHECK_GL_ERROR(glUniformBlockBinding(mProgramHandle, uniformBlockIndex, uniformBlockBinding));
  }
}
