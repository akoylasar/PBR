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
    std::unique_ptr<ShaderProgram> mPbrProgram;
    GpuMesh mCubeMesh;
    GpuMesh mSphereMesh;
    std::atomic<ImageData*> mImage = nullptr;
    GLuint mIrradianceMap;
    Neon::Vec3f mAlbedo = Neon::Vec3f(0.98, 0.96, 0.99);
    float mMetallic = 0.1f;
    float mRoughness = 0.8f;
    float mAo = 0.005;
    Neon::Vec3f mLightColor = Neon::Vec3f(1.0f);
    float mMixFactor = 0.0f;
    bool mInitialised = false;
  };
}
