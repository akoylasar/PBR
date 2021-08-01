/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#pragma once

#include <atomic>

#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

namespace Akoylasar
{
  class IBLScene
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
    void setupIrradianceMap();
    void setupPrefilterEnvMap();
    void setupBrdLUT();
    void drawUI(double deltaTime);
    static void renderToCubeMap(GLuint inputTexture,
                                bool isCubeMap,
                                GLuint outputTexture,
                                unsigned int width,
                                unsigned int height,
                                const ShaderProgram& program,
                                const GpuMesh& cubeMesh,
                                int mip);

  private:
    GLuint mEnvironmentTexture;
    std::unique_ptr<ShaderProgram> mBackgroundProgram;
    std::unique_ptr<ShaderProgram> mPrefilterEnvProgram;
    std::unique_ptr<ShaderProgram> mPbrProgram;
    GpuMesh mCubeMesh;
    GpuMesh mSphereMesh;
    std::atomic<ImageData*> mImage = nullptr;
    GLuint mIrradianceMap;
    GLuint mPrefilterMap;
    GLuint mBrdfLUT;
    Neon::Vec3f mAlbedo = Neon::Vec3f(0.98, 0.96, 0.99);
    float mMetallic = 0.1f;
    float mRoughness = 0.8f;
    float mAo = 0.005;
    bool mInitialised = false;
  };
}
