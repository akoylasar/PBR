/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#include "IBL.hpp"

#include <thread>
#include <array>

#include <stb_image.h>

#include <imgui.h>

#include <Neon.hpp>

#include "Common.hpp"

namespace
{
  const GLuint kMatricesUniformBlockBinding = 0;
  const char* const kMatricesUbName = "ubMatrices";
}

namespace Akoylasar
{
  void IBLScene::initialise()
  {
    // Load shader sources from disk.
    ProgramInfo backgroundProgramInfo {std::make_pair("shaders/background.vs", ""), std::make_pair("shaders/background.fs", "")};
    ProgramInfo pbrProgramInfo {std::make_pair("shaders/ibl.vs", ""), std::make_pair("shaders/ibl.fs", "")};
    std::array<ProgramInfo*, 2> programInfos {&backgroundProgramInfo, &pbrProgramInfo};
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

    // Create GPU shaders.
    GLuint matricesBlockIndex;

    // Setup background program
    mBackgroundProgram = std::make_unique<ShaderProgram>(backgroundProgramInfo.at(0).second, backgroundProgramInfo.at(1).second);
    matricesBlockIndex = mBackgroundProgram->getUniformBlockIndex(kMatricesUbName);
    mBackgroundProgram->setUniformBlockBinding(matricesBlockIndex, kMatricesUniformBlockBinding);
    
    // Setup PBR program
    mPbrProgram = std::make_unique<ShaderProgram>(pbrProgramInfo.at(0).second, pbrProgramInfo.at(1).second);
    matricesBlockIndex = mPbrProgram->getUniformBlockIndex(kMatricesUbName);
    mPbrProgram->setUniformBlockBinding(matricesBlockIndex, kMatricesUniformBlockBinding);
    
    CHECK_GL_ERROR(glGenTextures(1, &mEnvironmentTexture));
    CHECK_GL_ERROR(glGenTextures(1, &mPrefilterMap));
    CHECK_GL_ERROR(glGenTextures(1, &mIrradianceMap));
    CHECK_GL_ERROR(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

    const auto cubeMesh = Mesh::buildCube();
    mCubeMesh = GpuMesh::createGpuMesh(*cubeMesh);
    const auto sphereMesh = Mesh::buildSphere(1.5, 256, 256);
    mSphereMesh = GpuMesh::createGpuMesh(*sphereMesh);
    
    // Launch a separate thread to load image from disk without blocking main app.
    std::thread t(&IBLScene::loadAssets, this);
    t.detach();
  }
  
  void IBLScene::render(double deltaTime, const Camera& camera)
  {
    if (mInitialised)
    {
      // Draw background
      mBackgroundProgram->use();
      CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0));
      CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mEnvironmentTexture));
      mBackgroundProgram->setIntUniform(mBackgroundProgram->getUniformLocation("sBackground"), 0); // GL_TEXTURE0
      mCubeMesh.draw();
      
      // Draw sphere
      mPbrProgram->use();
      CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0));
      CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap));
      CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE1));
      CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMap));
      CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE2));
      CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mBrdfLUT));
      mPbrProgram->setVec3fUniform(mPbrProgram->getUniformLocation("uAlbedo"), mAlbedo);
      mPbrProgram->setFloatUniform(mPbrProgram->getUniformLocation("uMetallic"), mMetallic);
      mPbrProgram->setFloatUniform(mPbrProgram->getUniformLocation("uRoughness"), mRoughness);
      mPbrProgram->setFloatUniform(mPbrProgram->getUniformLocation("uAo"), mAo);
      mPbrProgram->setVec3fUniform(mPbrProgram->getUniformLocation("uCameraPos"), camera.getOrigin());
      mPbrProgram->setIntUniform(mPbrProgram->getUniformLocation("sIrradianceMap"), 0); // GL_TEXTURE1
      mPbrProgram->setIntUniform(mPbrProgram->getUniformLocation("sPrefilterMap"), 1); // GL_TEXTURE2
      mPbrProgram->setIntUniform(mPbrProgram->getUniformLocation("sBrdf"), 2); // GL_TEXTURE2
      mSphereMesh.draw();
    }
    else
    {
      ImageData* image = mImage.exchange(nullptr, std::memory_order_acq_rel);
      if (image)
      {
        steupResources(image);
        mInitialised = true;
      }
    }
  }
  
  void IBLScene::drawUI(double deltaTime)
  {
    if (mInitialised)
    {
      ImGui::ColorEdit3("Albedo", reinterpret_cast<float*>(&mAlbedo));
      ImGui::SliderFloat("Metallic", &mMetallic, 0, 1);
      ImGui::SliderFloat("Roughness", &mRoughness, 0, 1);
      ImGui::Separator();
      ImGui::SliderFloat("AO", &mAo, 0, 1.0);
    }
  }
  
  void IBLScene::shutdown()
  {
    if (mInitialised)
    {
      GpuMesh::releaseGpuMesh(mCubeMesh);
      
      mBackgroundProgram.release();
      
      CHECK_GL_ERROR(glDeleteTextures(1, &mEnvironmentTexture));
      CHECK_GL_ERROR(glDeleteTextures(1, &mPrefilterMap));
      CHECK_GL_ERROR(glDeleteTextures(1, &mBrdfLUT));
      mInitialised = false;
    }
  }
  
  void IBLScene::loadAssets()
  {
    // Load image from disk and create a GPU texture from it.
    std::filesystem::path imagePath {"images/Barce_Rooftop_C_3k.hdr"};
    int w, h, numComps;
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(imagePath.c_str(), &w, &h, &numComps, 0);
    if (!data)
    {
      std::cerr << "Failed to load texture with path " << imagePath << std::endl;
      return;
    }
    ImageData* image = new ImageData;
    image->width = w;
    image->height = h;
    image->image = data;
    mImage.store(image, std::memory_order_release);
  }
  
  void IBLScene::steupResources(ImageData* image)
  {
    setupBackgroundTexture(image);

    // Save viewport size.
    GLint viewPort[4];
    CHECK_GL_ERROR(glGetIntegerv(GL_VIEWPORT, viewPort));

    setupIrradianceMap();
    setupPrefilterEnvMap();
    setupBrdLUT();
    
    stbi_image_free(image->image);
    delete image;

    // Restore viewport size.
    CHECK_GL_ERROR(glViewport(viewPort[0], viewPort[1], viewPort[2], viewPort[3]));
  }
  
  void IBLScene::setupBackgroundTexture(ImageData* image)
  {
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mEnvironmentTexture));
    CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D,
                                0, // level
                                GL_RGB16F, // internal format
                                image->width,
                                image->height,
                                0, // border
                                GL_RGB,
                                GL_FLOAT, // data format
                                image->image));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  }

  void IBLScene::setupIrradianceMap()
  {
    ProgramInfo irradianceProgramInfo {std::make_pair("shaders/passThrough.vs", ""), std::make_pair("shaders/irradianceComputer.fs", "")};
    for (auto& pair : irradianceProgramInfo)
    {
      auto& path = pair.first;
      auto& str = pair.second;
      if (Common::readToString(path, str))
      {
        std::cerr << "Failed to load shader with path " << path << std::endl;
        return;
      }
    }

    auto program = std::make_unique<ShaderProgram>(irradianceProgramInfo.at(0).second, irradianceProgramInfo.at(1).second);
    program->use();

    // Generate the texture for prefilter map.
    // Allocate size for the cubemap sides and configure its sampler.
    constexpr int kMapSize = 32;
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap));
    for (unsigned int i = 0; i < 6; ++i)
      CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, kMapSize, kMapSize, 0, GL_RGB, GL_FLOAT, nullptr));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    renderToCubeMap(mEnvironmentTexture, false, mIrradianceMap, kMapSize, kMapSize, *program, mCubeMesh, 0);

    program.reset();  
  }
  
  void IBLScene::setupPrefilterEnvMap()
  {
    ProgramInfo prefilterProgramInfo {std::make_pair("shaders/passThrough.vs", ""), std::make_pair("shaders/prefilterEnvMap.fs", "")};
    for (auto& pair : prefilterProgramInfo)
    {
      auto& path = pair.first;
      auto& str = pair.second;
      if (Common::readToString(path, str))
      {
        std::cerr << "Failed to load shader with path " << path << std::endl;
        return;
      }
    }

    auto program = std::make_unique<ShaderProgram>(prefilterProgramInfo.at(0).second, prefilterProgramInfo.at(1).second);
    program->use();

    // Generate the texture for prefilter map.
    // Allocate size for the cubemap sides and configure its sampler.
    constexpr int kMapSize = 128;
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMap));
    for (unsigned int i = 0; i < 6; ++i)
      CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, kMapSize, kMapSize, 0, GL_RGB, GL_FLOAT, nullptr));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)); // needs to be GL_LINEAR_MIPMAP_LINEAR otherwise no mips are generated.
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    CHECK_GL_ERROR(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

    // Compute the prefilter map. Each mip level corresponding to a certain roughness value.
    constexpr int kMipLevels = 5;
    for (int mip = 0; mip < kMipLevels; ++mip)
    {
      const int size = kMapSize >> mip;
      const float roughness = mip / float(kMipLevels - 1);
      program->setFloatUniform(program->getUniformLocation("uRoughness"), roughness);
      renderToCubeMap(mEnvironmentTexture, false, mPrefilterMap, size, size, *program, mCubeMesh, mip);
    }

    program.reset();
  }
  
  void IBLScene::renderToCubeMap(GLuint inputTexture,
                                 bool isCubeMap,
                                 GLuint outputTexture,
                                 unsigned int width,
                                 unsigned int height,
                                 const ShaderProgram& program,
                                 const GpuMesh& cubeMesh,
                                 const int mip)
  {
    constexpr int kNumCubmapFaces = 6;

    // Create FBO and RBO.
    GLuint fbo, rbo;
    CHECK_GL_ERROR(glGenFramebuffers(1, &fbo));
    CHECK_GL_ERROR(glGenRenderbuffers(1, &rbo));

    // Setup the framebuffer state.
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    // Allocate size for the RBO.
    CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
    // Bind the RBO to the FBO.
    CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo));
    
    GLenum status;
    CHECK_GL_ERROR(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
    DEBUG_ASSERT_MSG(status == GL_FRAMEBUFFER_COMPLETE, "Invalid framebuffer");
    
    CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0));
    CHECK_GL_ERROR(glBindTexture(isCubeMap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, inputTexture));
    program.setIntUniform(program.getUniformLocation("sBackground"), 0);
    
    // Resize viewport.
    CHECK_GL_ERROR(glViewport(0, 0, width, height));

    // Setup projection and view matrices.
    Neon::Mat4f proj = Neon::makePerspective((float)Neon::kPi / 2.0f, 1.0f, 0.1f, 2.0f);
    std::array<Neon::Mat4f, kNumCubmapFaces> views
    {
      Neon::makeLookAt(Neon::Vec3f(0.0f), Neon::Vec3f(1.0f,  0.0f,  0.0f), Neon::Vec3f(0.0f, -1.0f,  0.0f)), // origin, look at, up
      Neon::makeLookAt(Neon::Vec3f(0.0f), Neon::Vec3f(-1.0f, 0.0f,  0.0f), Neon::Vec3f(0.0f, -1.0f,  0.0f)),
      Neon::makeLookAt(Neon::Vec3f(0.0f), Neon::Vec3f(0.0f,  1.0f,  0.0f), Neon::Vec3f(0.0f,  0.0f,  1.0f)),
      Neon::makeLookAt(Neon::Vec3f(0.0f), Neon::Vec3f(0.0f, -1.0f,  0.0f), Neon::Vec3f(0.0f,  0.0f, -1.0f)),
      Neon::makeLookAt(Neon::Vec3f(0.0f), Neon::Vec3f(0.0f,  0.0f,  1.0f), Neon::Vec3f(0.0f, -1.0f,  0.0)),
      Neon::makeLookAt(Neon::Vec3f(0.0f), Neon::Vec3f(0.0f,  0.0f, -1.0f), Neon::Vec3f(0.0f, -1.0f,  0.0f))
    };
    
    // Render.
    GLuint projLoc = program.getUniformLocation("uProjection");
    program.setMat4fUniform(projLoc, proj);
    GLuint viewLoc = program.getUniformLocation("uView");
    for (int i = 0; i < kNumCubmapFaces; ++i)
    {
      program.setMat4fUniform(viewLoc, views[i]);
      CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, outputTexture, mip));
      CHECK_GL_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
      cubeMesh.draw();
    }
    
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    CHECK_GL_ERROR(glDeleteRenderbuffers(1, &rbo));
    CHECK_GL_ERROR(glDeleteFramebuffers(1, &fbo));
  }
  
  void IBLScene::setupBrdLUT()
  {
    // Solve the brdf integral and wirte the results in a 2d texture.
    
    // Load shader source and create them.
    ProgramInfo brdfProgramInfo {std::make_pair("shaders/passThroughNoTransform.vs", ""), std::make_pair("shaders/brdf.fs", "")};
    for (auto& pair : brdfProgramInfo)
    {
      auto& path = pair.first;
      auto& str = pair.second;
      if (Common::readToString(path, str))
      {
        std::cerr << "Failed to load shader with path " << path << std::endl;
        return;
      }
    }
    auto program = std::make_unique<ShaderProgram>(brdfProgramInfo.at(0).second, brdfProgramInfo.at(1).second);
    program->use();
    
    const auto quadGeom = Mesh::buildQuad();
    GpuMesh quad = GpuMesh::createGpuMesh(*quadGeom);
    
    // Prepare FBO and RBO.
    const int size = 512;
    GLuint fbo, rbo;
    CHECK_GL_ERROR(glGenFramebuffers(1, &fbo));
    CHECK_GL_ERROR(glGenRenderbuffers(1, &rbo));
    // Setup the framebuffer state.
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    // Allocate size for the RBO.
    CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size));
    // Bind the RBO to the FBO.
    CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo));
    GLenum status;
    CHECK_GL_ERROR(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
    DEBUG_ASSERT_MSG(status == GL_FRAMEBUFFER_COMPLETE, "Invalid framebuffer");
    
    // Prepare the texture.
    CHECK_GL_ERROR(glGenTextures(1, &mBrdfLUT));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mBrdfLUT));
    CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, size, size, 0, GL_RG, GL_FLOAT, nullptr));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Resize viewport.
    CHECK_GL_ERROR(glViewport(0, 0, size, size));
    
    // Render
    CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBrdfLUT, 0));
    CHECK_GL_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    quad.draw();
    
    // Clean up RBO, FBO and GPU mesh.
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    CHECK_GL_ERROR(glDeleteRenderbuffers(1, &rbo));
    CHECK_GL_ERROR(glDeleteFramebuffers(1, &fbo));
    GpuMesh::releaseGpuMesh(quad);
    // program.reset();
  }
}
