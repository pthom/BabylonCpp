#ifdef IMGUI_RUNNER_USE_GLFW
#include "runner_glfw.h"

#include "imgui_utils/imgui_runner/runner_glfw.h"
#include "imgui_utils/imgui_runner/glad_callbacks.h"
#include <GLFW/glfw3.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <stdexcept>
#include <sstream>

// SDL + OpenGL3 + glad

namespace ImGui {
namespace ImGuiRunner {


static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void RunnerGlfw::InitBackend()
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    throw std::runtime_error("RunnerGlfw::InitBackend failed");
}

void RunnerGlfw::Select_Gl_Version()
{
  // Draw smooth line with antialias
  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

  // Initialize OpenGL context
  // OpenGL ES 3.0
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
}

std::string RunnerGlfw::GlslVersion()
{
  const char* glsl_version = "#version 300 es"; //  WebGL 2.0
  return glsl_version;
}

void RunnerGlfw::CreateWindowAndContext()
{
  // Check if full screen mode is requested
  if (mBackendFullScreen)
  {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    mWindow = glfwCreateWindow(mode->width, mode->height, mBackendWindowTitle.c_str(), monitor, nullptr);
  }
  else
  {
    mWindow = glfwCreateWindow((int)mBackendWindowSize.x, (int)mBackendWindowSize.y, mBackendWindowTitle.c_str(), NULL, NULL);
  }
  if ( (mBackendWindowPosition.x >= 0.f ) && (mBackendWindowPosition.y >= 0.f))
    glfwSetWindowPos(mWindow, (int)mBackendWindowPosition.x , (int)mBackendWindowPosition.y);

  if (mWindow == NULL)
  {
    glfwTerminate();
    throw std::runtime_error("RunnerGlfw::CreateWindowAndContext failed");
  }
  glfwMakeContextCurrent(mWindow);
  glfwSwapInterval(1); // Enable vsync
}

void RunnerGlfw::InitGlLoader()
{
#ifndef __EMSCRIPTEN__
//  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
//    throw std::runtime_error("gladLoadGLLoader: Failed");
//  if (!GLAD_GL_VERSION_3_3)
//    throw(std::runtime_error("GLAD could not initialize OpenGl 3.3"));

  // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
  bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
  bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
  bool err = gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
#else
  bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
  if (err)
  {
    throw std::runtime_error("RunnerGlfw::InitGlLoader(): Failed to initialize OpenGL loader!");
  }
  if (!GLAD_GL_ES_VERSION_3_0)
    throw(std::runtime_error("GLAD could not initialize OpenGl ES 3.0"));
#endif // #ifndef __EMSCRIPTEN__

#ifdef GLAD_DEBUG
  glad_set_pre_callback(glad_pre_call_callback);
  glad_set_post_callback(glad_post_call_callback);
#endif
}

void RunnerGlfw::SetupPlatformRendererBindings()
{
  ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
  ImGui_ImplOpenGL3_Init(GlslVersion().c_str());
}

void RunnerGlfw::PollEvents()
{
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
  // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
  glfwPollEvents();
  mExitRequired = glfwWindowShouldClose(mWindow);
}

void RunnerGlfw::NewFrame_OpenGl()
{
  ImGui_ImplOpenGL3_NewFrame();
}

void RunnerGlfw::NewFrame_Backend()
{
  ImGui_ImplGlfw_NewFrame();
}

void RunnerGlfw::Frame_OpenGl_ClearColor()
{
  auto & io = ImGui::GetIO();
  glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
  ImVec4 clear_color = mClearColor;
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
}

void RunnerGlfw::RenderDrawData_To_OpenGl()
{
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RunnerGlfw::UpdateAndRenderAdditionalPlatformWindows()
{
  GLFWwindow* backup_current_context = glfwGetCurrentContext();
  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();
  glfwMakeContextCurrent(backup_current_context);
}

void RunnerGlfw::Cleanup()
{
  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(mWindow);
  glfwTerminate();
}

void RunnerGlfw::SwapBuffers()
{
  glfwSwapBuffers(mWindow);
}

} // namespace ImGuiRunner
} // namespace ImGui
#endif // #ifdef IMGUI_RUNNER_USE_GLFW
