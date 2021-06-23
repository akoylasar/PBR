#include "GlfwApp.hpp"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <exception>

namespace Akoylasar
{
  GlfwApp::GlfwApp(const std::string& title, int width, int height, int major, int minor)
    : mWindow(nullptr),
      mDrawRequested(true)
  {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit())
      throw std::runtime_error("GlfwApp: Failed to initialise glfw.\n");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!mWindow)
    {
      glfwTerminate();
      std::string msg = "GlfwApp: Failed to create an OpenGL";
      msg += std::to_string(major) + "." + std::to_string(minor) + " window.\n";
      throw std::runtime_error(msg.c_str());
    }

    glfwSetWindowUserPointer(mWindow, this);
    setupCallbacks();

    glfwMakeContextCurrent(mWindow);

    if (gl3wInit())
    {
      glfwDestroyWindow(mWindow);
      glfwTerminate();
      throw std::runtime_error("GlfwApp: Failed to initialize gl3w.\n");
    }
  }

  GlfwApp::~GlfwApp()
  {
    glfwDestroyWindow(mWindow);
    glfwTerminate();
  }

  void GlfwApp::glfwErrorCallback(int error, const char* description)
  {
    std::cerr << "Error code: " << error << ". Error description: " << description << "\n";
  }

  void GlfwApp::setSwapInterval(int swapInterval)
  {
    glfwSwapInterval(swapInterval);
  }

  void GlfwApp::setStickyKeys(bool state)
  {
    glfwSetInputMode(mWindow, GLFW_LOCK_KEY_MODS, state);
  }

  void GlfwApp::swapBuffers()
  {
    glfwSwapBuffers(mWindow);
  }

  void GlfwApp::requestRedraw()
  {
    mDrawRequested = true;
  }

  void GlfwApp::run()
  {
    setup();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(mWindow))
    {
      double currentTime = glfwGetTime();
      const double deltaTime = currentTime - lastTime;
      
      if (mDrawRequested)
      {
        mDrawRequested = false;
        draw(deltaTime);
      }
      
      glfwPollEvents();

      lastTime = currentTime;
    }

    shutDown();
  }

  void GlfwApp::maximize()
  {
    glfwMaximizeWindow(mWindow);
  }

  void GlfwApp::setupCallbacks()
  {
    glfwSetFramebufferSizeCallback(mWindow, [](GLFWwindow* window, int width, int height)
    {
      GlfwApp* app = static_cast<GlfwApp*>(glfwGetWindowUserPointer(window));
      app->onFramebufferSize(width, height);
    });

    glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, double xPos, double yPos)
    {
      GlfwApp* app = static_cast<GlfwApp*>(glfwGetWindowUserPointer(window));
      app->onCursorPos(xPos, yPos);
    });

    glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int button, int action, int mods)
    {
      GlfwApp* app = static_cast<GlfwApp*>(glfwGetWindowUserPointer(window));
      app->onMouseButton(button, action, mods);
    });

    glfwSetScrollCallback(mWindow, [](GLFWwindow* window, double xOffset, double yOffset)
    {
      GlfwApp* app = static_cast<GlfwApp*>(glfwGetWindowUserPointer(window));
      app->onScroll(xOffset, yOffset);
    });

    glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int key, int scanCode, int action, int mods)
    {
      GlfwApp* app = static_cast<GlfwApp*>(glfwGetWindowUserPointer(window));
      app->onKey(key, scanCode, action, mods);
    });
  }
}
