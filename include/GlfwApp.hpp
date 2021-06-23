#pragma once

#include <string>

struct GLFWwindow;

namespace Akoylasar
{
  class GlfwApp
  {
  public:
    GlfwApp(const std::string& title, int width, int height, int major, int minor);
    virtual ~GlfwApp();

    void setSwapInterval(int swapInterval);
    void setStickyKeys(bool state);
    void swapBuffers();
    void requestRedraw();
    void run();
    void maximize();
    double getTimeMs();

    static void glfwErrorCallback(int error, const char* description);

  private:
    GlfwApp(const GlfwApp&) = delete;
    GlfwApp& operator=(const GlfwApp&) = delete;

  protected:
    // Window callbacks.
    virtual void onFramebufferSize(int width, int height) {}

    // Input callbacks.
    virtual void onCursorPos(double xPos, double yPos) {}
    virtual void onMouseButton(int button, int action, int mods) {}
    virtual void onScroll(double xOffset, double yOffset) {}

    virtual void onKey(int key, int scanCode, int action, int mods) {}

    virtual void setup() {}
    virtual void draw(double deltaTime) {}
    virtual void shutDown() {}

  private:
    void setupCallbacks();

  protected:
    GLFWwindow* mWindow;
    bool mDrawRequested;
  };
}
