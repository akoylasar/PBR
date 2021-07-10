#include "PbrScene.hpp"

#include <imgui.h>

#include "Common.hpp"

namespace
{
  const GLuint kMatricesUniformBlockBinding = 0;
  const char* const kMatricesUbName = "ubMatrices";

  const char* const kAlbedoUniformName = "uAlbedo";
  const char* const kMetallicUniformName = "uMetallic";
  const char* const kRoughnessUniformName = "uRoughness";
  const char* const kAoUniformName = "uAo";
  const char* const kCameraPosUniformName = "uCameraPos";
  const char* const kLightPositionsUniformName = "uLightPositions";
  const char* const kLightColorUniformName = "uLightColor";
  
  const char* const kDebugModeUniformName = "uMode";
  const char* kDebugModes[] = {"Position", "Normal", "Uvs"};
  
  const std::array<Neon::Vec3f, 4> kLightPositions{Neon::Vec3f{2.0f, 2.0f, 5.0f},
                                                   Neon::Vec3f{2.0f, -2.0f, 5.0f},
                                                   Neon::Vec3f{-2.0f, -2.0f, 5.0f},
                                                   Neon::Vec3f{-2.0f, 2.0f, 5.0f}
  };
}

namespace Akoylasar
{
   void PbrScene::initialise()
   {
     ProgramInfo pbrProgramInfo {std::make_pair("shaders/basicPbr.vs", ""), std::make_pair("shaders/basicPbr.fs", "")};
     ProgramInfo debugProgramInfo {std::make_pair("shaders/debug.vs", ""), std::make_pair("shaders/debug.fs", "")};
     
     std::array<ProgramInfo*, 2> programInfos {&pbrProgramInfo, &debugProgramInfo};
     for (auto programInfo : programInfos)
     {
       for (auto& pair : *programInfo)
       {
         auto& path = pair.first;
         auto& str = pair.second;
         if (Common::readToString(path, str))
         {
           std::cerr << "Failed to load shader with path " << path << std::endl;
           return;
         }
       }
     }
     
     GLuint matricesBlockIndex;

     mProgram = std::make_unique<ShaderProgram>(pbrProgramInfo.at(0).second, pbrProgramInfo.at(1).second);
     matricesBlockIndex = mProgram->getUniformBlockIndex(kMatricesUbName);
     mProgram->setUniformBlockBinding(matricesBlockIndex, kMatricesUniformBlockBinding);
     
     mDebugProgram = std::make_unique<ShaderProgram>(debugProgramInfo.at(0).second, debugProgramInfo.at(1).second);
     matricesBlockIndex = mDebugProgram->getUniformBlockIndex(kMatricesUbName);
     mDebugProgram->setUniformBlockBinding(matricesBlockIndex, kMatricesUniformBlockBinding);
     
     const auto sphereMesh = Mesh::buildSphere(3.0, 256, 256);
     mGpuMesh = GpuMesh::createGpuMesh(*sphereMesh);
     
     mAlbedoUniformLoc = mProgram->getUniformLocation(kAlbedoUniformName);
     mMetallicUniformLoc = mProgram->getUniformLocation(kMetallicUniformName);
     mRoughnessUniformLoc = mProgram->getUniformLocation(kRoughnessUniformName);
     mAoUniformLoc = mProgram->getUniformLocation(kAoUniformName);
     mCameraPosUniformLoc = mProgram->getUniformLocation(kCameraPosUniformName);
     mLightPositionsUniformLoc = mProgram->getUniformLocation(kLightPositionsUniformName);
     mLightColorUniformLoc = mProgram->getUniformLocation(kLightColorUniformName);
     
     mDebugModeUniformLoc = mDebugProgram->getUniformLocation(kDebugModeUniformName);
  
     mInitialised = true;
   }
   
   void PbrScene::render(double deltaTime, const Camera& camera)
   {
     if (mInitialised)
     {
       if (mRenderDebug)
       {
         mDebugProgram->use();
         mDebugProgram->setIntUniform(mDebugModeUniformLoc, mDebugMode);
       }
       else
       {
         mProgram->use();
         mProgram->setVec3fUniform(mAlbedoUniformLoc, mAlbedo);
         mProgram->setFloatUniform(mMetallicUniformLoc, mMetallic);
         mProgram->setFloatUniform(mRoughnessUniformLoc, mRoughness);
         mProgram->setFloatUniform(mAoUniformLoc, mAo);
         mProgram->setVec3fUniform(mCameraPosUniformLoc, camera.getOrigin());
         mProgram->setVec3fArrayUniform<4>(mLightPositionsUniformLoc, kLightPositions);
         mProgram->setVec3fUniform(mLightColorUniformLoc, mLightColor);
       }
       
       if (mRenderDebug && mShowWireframe)
         CHECK_GL_ERROR(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
       else
         CHECK_GL_ERROR(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
       
       mGpuMesh.draw();
     }
   }
   
   void PbrScene::drawUI(double deltaTime)
   {
     if (mInitialised)
     {
       ImGui::Checkbox("Debug", &mRenderDebug);
       ImGui::Separator();
       if (mRenderDebug)
       {
         const int n = sizeof(kDebugModes) / sizeof(*kDebugModes);
         ImGui::ListBox("Mode", &mDebugMode, kDebugModes, n);
         ImGui::Checkbox("Wireframe", &mShowWireframe);
       }
       else
       {
         ImGui::ColorEdit3("Albedo", reinterpret_cast<float*>(&mAlbedo));
         ImGui::SliderFloat("Metallic", &mMetallic, 0, 1);
         ImGui::SliderFloat("Roughness", &mRoughness, 0, 1);
         ImGui::Separator();
         ImGui::SliderFloat("AO", &mAo, 0, 0.1);
         ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&mLightColor));
       }
     }
   }
   
   void PbrScene::shutdown()
   {
     if (mInitialised)
     {
       GpuMesh::releaseGpuMesh(mGpuMesh);
       
       mDebugProgram.reset();
       mProgram.reset();
       
       mInitialised = false;
     }
   }
}
