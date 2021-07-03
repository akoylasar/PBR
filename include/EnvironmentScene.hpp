#pragma once

#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

namespace Akoylasar
{
  class EnvironmentScene
  {
  public:
    void initialise();
    void render(double deltaTime, const Camera& camera);
    void shutdown();
    
  private:
    private:
    GLuint mTexture;
    std::unique_ptr<ShaderProgram> mProgram;
    GpuMesh mGpuMesh;
    bool mInitialised = false;
  };
}
