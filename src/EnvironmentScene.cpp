#include "EnvironmentScene.hpp"

#include <thread>

#include <stb_image.h>

#include "Common.hpp"


namespace
{
  const GLuint kMatricesUniformBlockBinding = 0;
  const char* const kMatricesUbName = "ubMatrices";
}

namespace Akoylasar
{
  void EnvironmentScene::initialise()
  {
    // Load shader sources from disk.
    ProgramInfo environmentProgramInfo {std::make_pair("shaders/environment.vs", ""), std::make_pair("shaders/environment.fs", "")};
    for (auto& pair : environmentProgramInfo)
    {
      auto& path = pair.first;
      auto& str = pair.second;
      if (Common::readToString(path, str))
      {
        std::cerr << "Failed to load shader with path " << path << std::endl;
        return;
      }
    }
        
    // Launch a separate thread to load image from disk without blocking main app.
    std::thread t(&EnvironmentScene::loadImage, this);
    t.detach();

    // Create GPU shaders.
    GLuint matricesBlockIndex;

    mProgram = std::make_unique<ShaderProgram>(environmentProgramInfo.at(0).second, environmentProgramInfo.at(1).second);
    matricesBlockIndex = mProgram->getUniformBlockIndex(kMatricesUbName);
    mProgram->setUniformBlockBinding(matricesBlockIndex, kMatricesUniformBlockBinding);
    
    const auto cubeMesh = Mesh::buildCube();
    mGpuMesh = GpuMesh::createGpuMesh(*cubeMesh);
  }
  
  void EnvironmentScene::render(double deltaTime, const Camera& camera)
  {
    if (mInitialised)
    {
      CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0));
      CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mTexture));

      mProgram->use();
      CHECK_GL_ERROR(glBindVertexArray(mGpuMesh.vao));
      CHECK_GL_ERROR(glDrawElements(mGpuMesh.drawMode,
                                    mGpuMesh.indexCount,
                                    GL_UNSIGNED_INT,
                                    nullptr));
    }
    else
    {
      ImageData* image = mImage.exchange(nullptr, std::memory_order_acq_rel);
      if (image)
      {
        createGpuTexture(image);
        mInitialised = true;
      }
    }
  }
  
  void EnvironmentScene::shutdown()
  {
    if (mInitialised)
    {
      GpuMesh::releaseGpuMesh(mGpuMesh);
      
      mProgram.release();
      
      CHECK_GL_ERROR(glDeleteTextures(1, &mTexture));
      mInitialised = false;
    }
  }
  
  void EnvironmentScene::loadImage()
  {
    // Load image from disk and create a GPU texture from it.
    std::filesystem::path imagePath {"images/Barce_Rooftop_C_3k.hdr"};
    int w, h, numComps;
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
  
  void EnvironmentScene::createGpuTexture(ImageData* image)
  {
    CHECK_GL_ERROR(glGenTextures(1, &mTexture));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mTexture));
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
    stbi_image_free(image->image);
    delete image;
  }
}
