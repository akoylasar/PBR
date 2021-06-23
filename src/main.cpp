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
    const auto sphereMesh = Mesh::buildSphere(1.0, 32, 32);
    mGpuMesh = GpuMesh::createGpuMesh(*sphereMesh);
    mViewUniformLoc = mProgram->getUniformLocation(kViewUniformName);
    mProjUniformLoc = mProgram->getUniformLocation(kProjectionUniformName);
    mInitialised = true;
  }
  
  void render(double deltaTime, const Camera& camera)
  {
    if (mInitialised)
    {
      mProgram->use();
      mProgram->setMat4fUniform(mViewUniformLoc, camera.getView());
      mProgram->setMat4fUniform(mProjUniformLoc, camera.getProjection());
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
    mMainScene->initialise();
  }
  
  void onFramebufferSize(int width, int height) override
  {
    CHECK_GL_ERROR(glViewport(0, 0, width, height));
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
    ImGui::Text("UI rendering took %f(ms)", uiMs);

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
