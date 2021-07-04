#pragma once

#include <atomic>

#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

namespace Akoylasar
{
  class EnvironmentScene
  {
    private:
      struct ImageData
      {
        int width, height;
        float* image;
      };
    
  public:
    void initialise();
    void render(double deltaTime, const Camera& camera);
    void shutdown();
    void loadImage();
    void createGpuTexture(ImageData* image);

  private:
    GLuint mTexture;
    std::unique_ptr<ShaderProgram> mProgram;
    GpuMesh mGpuMesh;
    std::atomic<ImageData*> mImage = nullptr;
    bool mInitialised = false;
  };
}
