#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <utility>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GlfwApp.hpp"
#include "Profiler.hpp"
#include "Debug.hpp"
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Ubo.hpp"

using namespace Akoylasar;
using ShaderInfo = std::pair<std::filesystem::path, std::string>;
using ProgramInfo = std::array<ShaderInfo, 2>;

namespace
{
  constexpr int kWindowWidth = 712;
  constexpr int kWindowHeight = 712;
  constexpr int kGlMajor = 4;
  constexpr int kGlMinor = 1;
  const Neon::Vec3f kCameraOrigin {0.0f, 0.0f, 5.0f};
  const Neon::Vec3f kCameraLookAt {0.0f, 0.0f, 0.0f};
  const Neon::Vec3f kCameraUp {0.0f, 1.0f, 0.0f};
  constexpr float kFovy = 75.0f;
  constexpr float kAspect = (float)kWindowWidth / kWindowHeight;
  constexpr float kNear = 0.3f;
  constexpr float kFar = 1000.0f;
  
  const GLuint kMatricesUniformBlockBinding = 0;
  const std::string kMatricesUbName = "ubMatrices";

  const std::string kAlbedoUniformName = "uAlbedo";
  const std::string kMetallicUniformName = "uMetallic";
  const std::string kRoughnessUniformName = "uRoughness";
  const std::string kAoUniformName = "uAo";
  const std::string kCameraPosUniformName = "uCameraPos";
  const std::string kLightPositionsUniformName = "uLightPositions";
  const std::string kLightColorUniformName = "uLightColor";
  
  const std::string kDebugModeUniformName = "uMode";
  const char* kDebugModes[] = {"Position", "Normal", "Uvs"};
  
  const std::array<Neon::Vec3f, 4> kLightPositions{Neon::Vec3f{2.0f, 2.0f, 5.0f},
    																							 Neon::Vec3f{2.0f, -2.0f, 5.0f},
                                                   Neon::Vec3f{-2.0f, -2.0f, 5.0f},
                                                   Neon::Vec3f{-2.0f, 2.0f, 5.0f}
  };

  bool readToString(const std::filesystem::path& file, std::string& output)
  {
    std::ifstream strm {file};
    if (!strm.is_open())
      return true;
    std::stringstream buffer;
    buffer << strm.rdbuf();
    output = buffer.str();
    return false;
  }
}

class EnvironmentScene
{
public:
  void initialise()
  {
    // Load shader sources from disk.
    ProgramInfo environmentProgramInfo {std::make_pair("shaders/environment.vs", ""), std::make_pair("shaders/environment.fs", "")};
    for (auto& pair : environmentProgramInfo)
    {
      auto& path = pair.first;
      auto& str = pair.second;
      if (readToString(path, str))
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
  
  void render(double deltaTime, const Camera& camera)
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
  
  void shutdown()
  {
    if (mInitialised)
    {
      GpuMesh::releaseGpuMesh(mGpuMesh);
      
      mProgram.release();
      
      CHECK_GL_ERROR(glDeleteTextures(1, &mTexture));
      mInitialised = false;
    }
  }
private:
  private:
  GLuint mTexture;
  std::unique_ptr<ShaderProgram> mProgram;
  GpuMesh mGpuMesh;
  bool mInitialised = false;
};

class PbrScene
{
public:
  void initialise()
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
        if (readToString(path, str))
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
    
    const auto sphereMesh = Mesh::buildSphere(1.0, 256, 256);
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
  
  void render(double deltaTime, const Camera& camera)
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
      
      CHECK_GL_ERROR(glBindVertexArray(mGpuMesh.vao));
      CHECK_GL_ERROR(glDrawElements(mGpuMesh.drawMode,
                                    mGpuMesh.indexCount,
                                    GL_UNSIGNED_INT,
                                    nullptr));
    }
  }
  
  void drawUI(double deltaTime)
  {
    if (mInitialised)
    {
      bool open = true;
      ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                     ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav;
      if (ImGui::Begin("Controls", &open, windowFlags))
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
      ImGui::End();
    }
  }
  
  void shutdown()
  {
    if (mInitialised)
    {
      GpuMesh::releaseGpuMesh(mGpuMesh);
      
      mDebugProgram.reset();
      mProgram.reset();
      
      mInitialised = false;
    }
  }
  
private:
  std::unique_ptr<ShaderProgram> mProgram;
  std::unique_ptr<ShaderProgram> mDebugProgram;
  GpuMesh mGpuMesh;
    
 	GLint mAlbedoUniformLoc;
 	GLint mMetallicUniformLoc;
 	GLint mRoughnessUniformLoc;
  GLint mAoUniformLoc;
 	GLint mCameraPosUniformLoc;
 	GLint mLightPositionsUniformLoc;
 	GLint mLightColorUniformLoc;
  
 	GLint mDebugModeUniformLoc;
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

class MainApp : public GlfwApp
{
public:
  MainApp(const std::string& title, int width, int height, int major, int minor)
  : GlfwApp(title, width, height, major, minor),
  	mProfiler(nullptr),
  	mUiTs(nullptr),
  	mCamera(std::make_unique<Camera>(kCameraOrigin,
                                     kCameraLookAt,
                                     kCameraUp,
                                     kFovy,
                                     kAspect,
                                     kNear,
                                     kFar)),
  	mMatricesUbo(nullptr),
  	mPbrScene(std::make_unique<PbrScene>()),
  	mEnvironmentScene(std::make_unique<EnvironmentScene>()),
  	mShowEnvironment(true)
  {}
  ~MainApp() override = default;
protected:
  void setup() override
  {
    setSwapInterval(1);
    
    // Setup dear ImGui.
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(mWindow, false);
    ImGui_ImplOpenGL3_Init("#version 410");
        
    mProfiler = std::make_unique<Profiler>();
    mUiTs = &mProfiler->createTimeStamp();
    
    CHECK_GL_ERROR(glEnable(GL_DEPTH_TEST));
    CHECK_GL_ERROR(glEnable(GL_CULL_FACE));
    
    // Setup matrices UBO
    const auto matricesBlockSize = 2 * sizeof(Neon::Mat4f);
    mMatricesUbo = Ubo::createUbo(matricesBlockSize, kMatricesUniformBlockBinding);

    mPbrScene->initialise();
    mEnvironmentScene->initialise();
  }
  
  void onFramebufferSize(int width, int height) override
  {
    CHECK_GL_ERROR(glViewport(0, 0, width, height));
    const float aspect = (float)width / height;
    mCamera->setProjection(kFovy, aspect, kNear, kFar);
  }
  
  void onScroll(double xOffset, double yOffset) override
  {
    mCamera->rotate(0.1 * xOffset, 0.1 * yOffset);
  }

  void draw(double deltaTime) override
  {
    CHECK_GL_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    
    Ubo::updateUbo(*mMatricesUbo, 0, sizeof(Neon::Mat4f), mCamera->getProjection().data());
    Ubo::updateUbo(*mMatricesUbo, sizeof(Neon::Mat4f), sizeof(Neon::Mat4f), mCamera->getView().data());

    if (mShowEnvironment)
    {
      glDepthFunc(GL_LEQUAL);
      mEnvironmentScene->render(deltaTime, *mCamera);
    }
    else
      glDepthFunc(GL_LESS);
    mPbrScene->render(deltaTime, *mCamera);

    drawUI(deltaTime);

    mProfiler->swapBuffers();
    swapBuffers();
    requestRedraw();
  }
  
  void shutDown() override
  {
    mEnvironmentScene.reset();
    mPbrScene.reset();
    
    Ubo::releaseUbo(*mMatricesUbo);
    mMatricesUbo.reset();
    
    // dear ImGui cleanup.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
    
private:
  void drawUI(double deltaTime)
  {
    mUiTs->begin();
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    double uiMs = mUiTs->getElapsedTime() * fromNsToMs;
    mPbrScene->drawUI(deltaTime);
    
    ImVec2 windowPos, windowPosPivot;
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    ImGui::SetNextWindowBgAlpha(0.35f);
    bool open = true;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
    															 ImGuiWindowFlags_AlwaysAutoResize |
    															 ImGuiWindowFlags_NoSavedSettings |
    															 ImGuiWindowFlags_NoFocusOnAppearing |
    															 ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("General", &open, windowFlags))
    {
      ImGui::Text("UI (GPU): %.2f(ms)", uiMs);
      ImGui::Separator();
      ImGui::Text("Frame time: %.2f(ms)", deltaTime * 1000.0);
      ImGui::Separator();
      ImGui::Checkbox("Show environment", &mShowEnvironment);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();
    
    mUiTs->end();
  }
private:
  std::unique_ptr<Profiler> mProfiler;
  TimeStamp* mUiTs;
  std::unique_ptr<Camera> mCamera;
  std::unique_ptr<Ubo> mMatricesUbo;
  std::unique_ptr<PbrScene> mPbrScene;
  std::unique_ptr<EnvironmentScene> mEnvironmentScene;
  bool mShowEnvironment;
};

int main()
{
  std::unique_ptr<GlfwApp> app;
  try
  {
    app = std::make_unique<MainApp>("PBR", kWindowWidth, kWindowHeight, kGlMajor, kGlMinor);
  }
  catch(const std::exception& e)
  {
    std::cout << e.what();
    return EXIT_FAILURE;
  }

  app->run();
  
  return EXIT_SUCCESS;
}
