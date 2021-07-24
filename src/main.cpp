#include <iostream>
#include <memory>

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
#include "Camera.hpp"
#include "Ubo.hpp"
#include "EnvironmentScene.hpp"
#include "PbrScene.hpp"
#include "IBL.hpp"

using namespace Akoylasar;

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
  const char* const kMatricesUbName = "ubMatrices";
}

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
  	mIBLScene(std::make_unique<IBLScene>())
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
    CHECK_GL_ERROR(glDepthFunc(GL_LEQUAL));

    // Setup matrices UBO
    const auto matricesBlockSize = 2 * sizeof(Neon::Mat4f);
    mMatricesUbo = Ubo::createUbo(matricesBlockSize, kMatricesUniformBlockBinding);

    mPbrScene->initialise();
    mEnvironmentScene->initialise();
    mIBLScene->initialise();
    
    clearGLErrors();
  }
  
  void onKey(int key, int scanCode, int action, int mods) override
  {
    mSceneIndex += static_cast<int>(key == GLFW_KEY_F && action == GLFW_PRESS);
    mSceneIndex %= 3;
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

    if (mSceneIndex == 0)
      mPbrScene->render(deltaTime, *mCamera);
    else if (mSceneIndex == 1)
      mEnvironmentScene->render(deltaTime, *mCamera);
    else if (mSceneIndex == 2)
      mIBLScene->render(deltaTime, *mCamera);

    drawUI(deltaTime);

    mProfiler->swapBuffers();
    swapBuffers();
    requestRedraw();
  }
  
  void shutDown() override
  {
    mEnvironmentScene.reset();
    mPbrScene.reset();
    mIBLScene.reset();
    
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
    }
    ImGui::End();
    
    if (mSceneIndex == 0)
      mPbrScene->drawUI(deltaTime);
    else if (mSceneIndex == 1)
      mEnvironmentScene->drawUI(deltaTime);
    else if (mSceneIndex == 2)
      mIBLScene->drawUI(deltaTime);
    
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
  std::unique_ptr<IBLScene> mIBLScene;
  int mSceneIndex = 0;
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
