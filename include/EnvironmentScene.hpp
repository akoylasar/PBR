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
    void steupResources(ImageData* image);
    void setupBackgroundTexture(ImageData* image);
    void setupIrradianceMap(ImageData* image);
    void drawUI(double deltaTime);
    static void renderToCubeMap(GLuint inputTexture,
                         				bool isCubeMap,
                         				GLuint outputTexture,
                         				unsigned int width,
                         				unsigned int height,
                         				const ShaderProgram& program,
                         				const GpuMesh& cubeMesh);

  private:
    GLuint mTexture;
    std::unique_ptr<ShaderProgram> mBackgroundProgram;
    GpuMesh mCubeMesh;
    std::atomic<ImageData*> mImage = nullptr;
    GLuint mIrradianceMap;
    float mMixFactor = 0.0f;
    GLuint mMixFactorUniformLoc;
    bool mInitialised = false;
  };
}
