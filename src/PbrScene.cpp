/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#include "PbrScene.hpp"

#include <imgui.h>

#include "Common.hpp"

namespace
{
  const GLuint kMatricesUniformBlockBinding = 0;
  const char* const kMatricesUbName = "ubMatrices";
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
  
     mInitialised = true;
   }
   
   void PbrScene::render(double deltaTime, const Camera& camera)
   {
     if (mInitialised)
     {
       if (mRenderDebug)
       {
         mDebugProgram->use();
         mDebugProgram->setIntUniform(mDebugProgram->getUniformLocation("uMode"), mDebugMode);
       }
       else
       {
         mProgram->use();
         mProgram->setVec3fUniform(mProgram->getUniformLocation("uAlbedo"), mAlbedo);
         mProgram->setFloatUniform(mProgram->getUniformLocation("uMetallic"), mMetallic);
         mProgram->setFloatUniform(mProgram->getUniformLocation("uRoughness"), mRoughness);
         mProgram->setFloatUniform(mProgram->getUniformLocation("uAo"), mAo);
         mProgram->setVec3fUniform(mProgram->getUniformLocation("uCameraPos"), camera.getOrigin());
         mProgram->setVec3fArrayUniform<4>(mProgram->getUniformLocation("uLightPositions"), kLightPositions);
         mProgram->setVec3fUniform(mProgram->getUniformLocation("uLightColor"), mLightColor);
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
