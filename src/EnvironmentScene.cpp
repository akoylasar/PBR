#include "EnvironmentScene.hpp"

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
    
    // Load image from disk and create a GPU texture from it.
    int width, height, numComps;
    std::filesystem::path imagePath {"images/Barce_Rooftop_C_3k.hdr"};
    float* image = stbi_loadf(imagePath.c_str(), &width, &height, &numComps, 0);
    if (!image)
    {
      std::cerr << "Failed to load texture with path " << imagePath << std::endl;
      return;
    }
    CHECK_GL_ERROR(glGenTextures(1, &mTexture));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, mTexture));
    CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D,
                                0, // level
                                GL_RGB16F, // internal format
                                width,
                                height,
                                0, // border
                                GL_RGB,
                                GL_FLOAT, // data format
                                image));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    stbi_image_free(image);

    // Create GPU shaders.
    GLuint matricesBlockIndex;

    mProgram = std::make_unique<ShaderProgram>(environmentProgramInfo.at(0).second, environmentProgramInfo.at(1).second);
    matricesBlockIndex = mProgram->getUniformBlockIndex(kMatricesUbName);
    mProgram->setUniformBlockBinding(matricesBlockIndex, kMatricesUniformBlockBinding);
    
    const auto cubeMesh = Mesh::buildCube();
    mGpuMesh = GpuMesh::createGpuMesh(*cubeMesh);
    
    mInitialised = true;
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
}
