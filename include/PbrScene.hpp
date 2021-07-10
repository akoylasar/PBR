#pragma once

#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

namespace Akoylasar
{
  class PbrScene
  {
  public:
    void initialise();
    void render(double deltaTime, const Camera& camera);
    void drawUI(double deltaTime);
    void shutdown();
    
  private:
    std::unique_ptr<ShaderProgram> mProgram;
    std::unique_ptr<ShaderProgram> mDebugProgram;
    GpuMesh mGpuMesh;

    int mDebugMode = 0;
    Neon::Vec3f mAlbedo = Neon::Vec3f(0.0, 0.15, 0.9);
    float mMetallic = 0.1f;
    float mRoughness = 0.8f;
    float mAo = 0.005;
    Neon::Vec3f mLightColor = Neon::Vec3f(1.0f);

    bool mShowWireframe = false;
    bool mInitialised = false;
    bool mRenderDebug = false;
  };
}
