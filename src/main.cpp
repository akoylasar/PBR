#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "GlfwApp.hpp"
#include "Profiler.hpp"
#include "Debug.hpp"
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

using namespace Akoylasar;

namespace
{
  constexpr int kWindowWidth = 512;
  constexpr int kWindowHeight = 512;
  constexpr int kGlMajor = 4;
  constexpr int kGlMinor = 1;
  const Neon::Vec3f kCameraOrigin {0.0f, 0.0f, 5.0f};
  const Neon::Vec3f kCameraLookAt {0.0f, 0.0f, 0.0f};
  const Neon::Vec3f kCameraUp {0.0f, 1.0f, 0.0f};
  constexpr float kFovy = 75.0f;
  constexpr float kAspect = (float)kWindowWidth / kWindowHeight;
  constexpr float kNear = 0.3f;
  constexpr float kFar = 1000.0f;
  const std::string kViewUniformName = "uView";
  const std::string kProjectionUniformName = "uProjection";
  const std::string kAlbedoUniformName = "uAlbedo";
  const std::string kMetallicUniformName = "uMetallic";
  const std::string kRoughnessUniformName = "uRoughness";
  const std::string kAoUniformName = "uAo";
  const std::string kCameraPosUniformName = "uCameraPos";
  const std::string kLightPositionsUniformName = "uLightPositions";
  const std::string kLightColorUniformName = "uLightColor";
  
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

class MainScene
{
public:
  void initialise()
  {
    const std::filesystem::path vsPath {"basicPbr.vs"};
    const std::filesystem::path fsPath {"basicPbr.fs"};
    std::string vs, fs;
    if (readToString(vsPath, vs))
    {
      std::cerr << "Failed to load vertex shader" << std::endl;
      return;
    }
    if (readToString(fsPath, fs))
    {
      std::cerr << "Failed to load fragment shader" << std::endl;
      return;
    }
    mProgram = std::make_unique<ShaderProgram>(vs, fs);
    const auto sphereMesh = Mesh::buildSphere(3.0, 256, 256);
    mGpuMesh = GpuMesh::createGpuMesh(*sphereMesh);
    
    mViewUniformLoc = mProgram->getUniformLocation(kViewUniformName);
    mProjUniformLoc = mProgram->getUniformLocation(kProjectionUniformName);
    
    mAlbedoUniformLoc = mProgram->getUniformLocation(kAlbedoUniformName);
    mMetallicUniformLoc = mProgram->getUniformLocation(kMetallicUniformName);
    mRoughnessUniformLoc = mProgram->getUniformLocation(kRoughnessUniformName);
    mAoUniformLoc = mProgram->getUniformLocation(kAoUniformName);
    mCameraPosUniformLoc = mProgram->getUniformLocation(kCameraPosUniformName);
    mLightPositionsUniformLoc = mProgram->getUniformLocation(kLightPositionsUniformName);
    mLightColorUniformLoc = mProgram->getUniformLocation(kLightColorUniformName);
    
    mInitialised = true;
  }
  
  void render(double deltaTime, const Camera& camera)
  {
    if (mInitialised)
    {
      mProgram->use();
      mProgram->setMat4fUniform(mViewUniformLoc, camera.getView());
      mProgram->setMat4fUniform(mProjUniformLoc, camera.getProjection());
      mProgram->setVec3fUniform(mAlbedoUniformLoc, mAlbedo);
      mProgram->setFloatUniform(mMetallicUniformLoc, mMetallic);
      mProgram->setFloatUniform(mRoughnessUniformLoc, mRoughness);
      mProgram->setFloatUniform(mAoUniformLoc, mAo);
      mProgram->setVec3fUniform(mCameraPosUniformLoc, camera.getOrigin());
      mProgram->setVec3fArrayUniform<4>(mLightPositionsUniformLoc, kLightPositions);
      mProgram->setVec3fUniform(mLightColorUniformLoc, mLightColor);
      CHECK_GL_ERROR(glBindVertexArray(mGpuMesh.vao));
      
      if (mShowWireframe)  CHECK_GL_ERROR(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
			else CHECK_GL_ERROR(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
      
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
        ImGui::ColorEdit3("Albedo", reinterpret_cast<float*>(&mAlbedo));
        ImGui::SliderFloat("Metallic", &mMetallic, 0, 1);
        ImGui::SliderFloat("Roughness", &mRoughness, 0, 1);
        ImGui::Separator();
        ImGui::SliderFloat("AO", &mAo, 0, 0.1);
        ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&mLightColor));
      }
      ImGui::End();
    }
  }
  
  void shutdown()
  {
    if (mInitialised)
    {
      GpuMesh::releaseGpuMesh(mGpuMesh);
      mProgram.release();
      mProgram = nullptr;
      mInitialised = false;
    }
  }
  
private:
  std::unique_ptr<ShaderProgram> mProgram;
  GpuMesh mGpuMesh;
  
  GLint mViewUniformLoc;
  GLint mProjUniformLoc;
 	GLint mAlbedoUniformLoc;
 	GLint mMetallicUniformLoc;
 	GLint mRoughnessUniformLoc;
  GLint mAoUniformLoc;
 	GLint mCameraPosUniformLoc;
 	GLint mLightPositionsUniformLoc;
 	GLint mLightColorUniformLoc;
  
  Neon::Vec3f mAlbedo = Neon::Vec3f(0.0, 0.15, 0.9);
  float mMetallic = 0.1f;
  float mRoughness = 0.8f;
  float mAo = 0.005;
  Neon::Vec3f mLightColor = Neon::Vec3f(1.0f);

  bool mShowWireframe = false;
  bool mInitialised = false;
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
  	mMainScene(std::make_unique<MainScene>())
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

    mMainScene->initialise();
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

    mMainScene->render(deltaTime, *mCamera);
    
    drawUI(deltaTime);

    mProfiler->swapBuffers();
    swapBuffers();
    requestRedraw();
  }
  
  void shutDown() override
  {
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
    mMainScene->drawUI(deltaTime);
    
    ImVec2 windowPos, windowPosPivot;
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    ImGui::SetNextWindowBgAlpha(0.35f);
    bool open = true;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
    															 ImGuiWindowFlags_AlwaysAutoResize |
    															 ImGuiWindowFlags_NoSavedSettings |
    															 ImGuiWindowFlags_NoFocusOnAppearing |
    															 ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("Frame statistics", &open, windowFlags))
    {
      ImGui::Text("UI (GPU): %.2f(ms)", uiMs);
      ImGui::Separator();
      ImGui::Text("Frame time: %.2f(ms)", deltaTime * 1000.0);
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
  std::unique_ptr<MainScene> mMainScene;
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
