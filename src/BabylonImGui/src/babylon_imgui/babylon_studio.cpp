#include <babylon/GL/framebuffer_canvas.h>
#include <babylon/babylon_imgui/babylon_logs_window.h>
#include <babylon/babylon_imgui/babylon_studio.h>
#include <babylon/cameras/free_camera.h>
#include <babylon/core/filesystem.h>
#include <babylon/core/system.h>
#include <babylon/samples/samples_info.h>
#include <babylon/inspector/components/actiontabs/action_tabs_component.h>
#include <babylon/interfaces/irenderable_scene_with_hud.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_utils/imgui_runner_babylon/runner_babylon.h>
#include <imgui_utils/icons_font_awesome_5.h>

#include <babylon/babylon_imgui/babylon_studio_layout.h>
#include <babylon/core/logging.h>

#ifdef _WIN32
#include <windows.h>
#endif

// #define BABYLON_LOG_TRACE_ON
#include <babylon/core/log_trace.h>

#include <iostream>

namespace BABYLON {

struct EmptyScene : public BABYLON::IRenderableScene {
  const char* getName() override
  {
    return "Empty Scene";
  }
  void initializeScene(BABYLON::ICanvas*, BABYLON::Scene* scene) override
  {
    auto camera = BABYLON::FreeCamera::New("camera1", Vector3(0.f, 0.f, 0.f), scene);
  }
};


class BabylonStudioApp {
public:
  BabylonStudioApp()
  {
    std::string exePath        = BABYLON::System::getExecutablePath();
    std::string exeFolder      = BABYLON::Filesystem::baseDir(exePath);
    std::string playgroundPath = exeFolder + "/../../../src/BabylonStudio/playground.cpp";
    playgroundPath             = BABYLON::Filesystem::absolutePath(playgroundPath);
    _playgroundCodeEditor.setFiles({playgroundPath});
    _playgroundCodeEditor.setLightPalette();
  }

  void RunApp(const std::shared_ptr<BABYLON::IRenderableScene>& initialScene,
              const BabylonStudioOptions& options)
  {
    _appContext._options                              = options;
    _appContext._options._appWindowParams.ShowMenuBar = true;

    auto showGuiLambda = [this]() -> bool {
      bool r = this->render();
      for (auto f : _appContext._options._heartbeatCallbacks)
        f();
      if (_appContext._options._playgroundCompilerCallback) {
        PlaygroundCompilerStatus playgroundCompilerStatus
          = _appContext._options._playgroundCompilerCallback();
        if (playgroundCompilerStatus._renderableScene)
          setRenderableScene(playgroundCompilerStatus._renderableScene);
        _appContext._isCompiling = playgroundCompilerStatus._isCompiling;
      }
      return r;
    };

    auto initSceneLambda = [=]() {
      this->initScene();
      this->setRenderableScene(initialScene);
    };

    _appContext._options._appWindowParams.InitialDockLayoutFunction = [this](ImGuiID mainDockId) {
      _studioLayout.PrepareLayout(mainDockId);
    };
    ImGuiUtils::ImGuiRunner::InvokeRunnerBabylon(
      _appContext._options._appWindowParams,
      showGuiLambda,
      initSceneLambda
    );
  }

private:
  void registerRenderFunctions()
  {
    static bool registered = false;
    if (registered)
      return;

    // clang-format off
    _studioLayout.registerGuiRenderFunction(
      DockableWindowId::Inspector,
      [this]() { 
        if (_appContext._inspector) 
          _appContext._inspector->render(); 
      });
        
    _studioLayout.registerGuiRenderFunction(
      DockableWindowId::Logs, 
      []() { BABYLON::BabylonLogsWindow::instance().render(); });
    
    _studioLayout.registerGuiRenderFunction(
      DockableWindowId::Scene3d, 
      [this]() { render3d(); });
    
    _studioLayout.registerGuiRenderFunction(
      DockableWindowId::SamplesCodeViewer,
      [this]() { _samplesCodeEditor.render(); });
    
    _studioLayout.registerGuiRenderFunction(
      DockableWindowId::SampleBrowser, 
      [this]() { _appContext.sampleListComponent().render(); });
    
#ifdef BABYLON_BUILD_PLAYGROUND
    _studioLayout.registerGuiRenderFunction(
      DockableWindowId::PlaygroundEditor,
      [this]() { renderPlayground(); });
#endif

    // clang-format on

    registered = true;
  }

  void initScene()
  {
    _appContext.sampleListComponent().OnNewRenderableScene
    = [=](std::shared_ptr<IRenderableScene> scene) {
        this->setRenderableScene(scene);
        _studioLayout.FocusWindow(DockableWindowId::Scene3d);
      };

    _appContext.sampleListComponent().OnEditFiles = [=](const std::vector<std::string>& files) {
      _samplesCodeEditor.setFiles(files);
      _studioLayout.setVisible(DockableWindowId::SamplesCodeViewer, true);
      _studioLayout.FocusWindow(DockableWindowId::SamplesCodeViewer);
    };

    _appContext.sampleListComponent().OnLoopSamples = [=](const std::vector<std::string>& samples) {
      _appContext._loopSamples.flagLoop      = true;
      _appContext._loopSamples.samplesToLoop = samples;
      _appContext._loopSamples.currentIdx    = 0;
    };

    _appContext._sceneWidget = std::make_unique<BABYLON::ImGuiSceneWidget>(ImVec2(640.f, 480.f));
    _appContext._sceneWidget->OnBeforeResize.push_back(
      [this]() { _appContext._inspector.release(); });
  }

  void prepareSceneInspector()
  {
    auto currentScene = _appContext._sceneWidget->getScene();
    if ((!_appContext._inspector) || (_appContext._inspector->scene() != currentScene)) {
      _appContext._inspector
        = std::make_unique<BABYLON::Inspector>(nullptr, _appContext._sceneWidget->getScene());
      _appContext._inspector->setScene(currentScene);
    }
  }

  // returns true if exit required
  bool renderMenu()
  {
    ImGui::GetCurrentWindow()->Flags = ImGui::GetCurrentWindow()->Flags | ImGuiWindowFlags_MenuBar;
    bool shallExit                   = false;
    if (ImGui::BeginMenuBar()) 
    {
      if (ImGui::BeginMenu("File")) 
      {
        if (ImGui::MenuItem("Quit"))
          shallExit = true;
        if (ImGui::MenuItem("Save Screenshot"))
          saveScreenshot();
        if (ImGui::MenuItem("ImGui demo window", nullptr, _appContext._imgui_show_demo_window))
          _appContext._imgui_show_demo_window = ! _appContext._imgui_show_demo_window;
        ImGui::EndMenu();
      }
      _studioLayout.renderMenu();

      renderStatus();
      ImGui::EndMenuBar();
    }

    return shallExit;
  }

  void renderStatus()
  {
    ImVec4 statusTextColor{ 0.8f, 0.8f, 0.3f, 1.f };
    if (_studioLayout.isVisible(DockableWindowId::Scene3d))
    {
      ImGui::SameLine(ImGui::GetIO().DisplaySize.x / 2.f);
      std::string sceneName = _appContext._sceneWidget->getRenderableScene()->getName();
      ImGui::TextColored(statusTextColor, "%s", sceneName.c_str());
    }

    ImGui::SameLine(ImGui::GetIO().DisplaySize.x - 70.f);
    ImGui::TextColored(statusTextColor, "FPS: %.1f", ImGui::GetIO().Framerate);
  }

  // renders the GUI. Returns true when exit required
  bool render()
  {
    if (_appContext._imgui_show_demo_window)
      ImGui::ShowDemoWindow(&_appContext._imgui_show_demo_window);

    static bool wasInitialLayoutApplied = false;
    if (!wasInitialLayoutApplied)
    {
      this->_studioLayout.ApplyLayoutMode(LayoutMode::SceneAndBrowser);
      wasInitialLayoutApplied = true;
    }

    prepareSceneInspector();
    registerRenderFunctions();

    bool shallExit = renderMenu();
    _studioLayout.renderGui();

    handleLoopSamples();

    if (_appContext._options._flagScreenshotOneSampleAndExit)
      return saveScreenshotAfterFewFrames();
    else
      return shallExit;
  }



  void setRenderableScene(std::shared_ptr<BABYLON::IRenderableScene> scene)
  {
    if (_appContext._inspector)
      _appContext._inspector->setScene(nullptr);
    _appContext._sceneWidget->setRenderableScene(scene);
    if (_appContext._inspector)
      _appContext._inspector->setScene(_appContext._sceneWidget->getScene());
  }

  void saveScreenshot(std::string sampleName = "")
  {
    if (sampleName.empty())
      sampleName = std::string(_appContext._sceneWidget->getRenderableScene()->getName());

    std::string filename = SamplesInfo::SampleScreenshotFile_Absolute(sampleName);
    int imageWidth = 200;
    int jpgQuality = 75;
    this->_appContext._sceneWidget->getCanvas()->saveScreenshotJpg(
      filename.c_str(), jpgQuality, imageWidth);
  }

  // Saves a screenshot after  few frames (returns true when done)
  [[nodiscard]] bool saveScreenshotAfterFewFrames()
  {
    _appContext._frameCounter++;
    if (_appContext._frameCounter < 30)
      return false;
    saveScreenshot(_appContext._options._sceneName);
    return true;
  }

  bool ButtonInOverlayWindow(const std::string& label, ImVec2 position, ImVec2 size)
  {
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;
    std::string id         = label + "##ButtonInOverlayWindow";
    ImGui::Begin(label.c_str(), nullptr, flags);
    bool clicked = ImGui::Button(label.c_str());
    ImGui::End();
    return clicked;
  }

  void renderHud(ImVec2 cursorScene3dTopLeft)
  {
    auto asSceneWithHud
      = dynamic_cast<IRenderableSceneWithHud*>(_appContext._sceneWidget->getRenderableScene());
    if (!asSceneWithHud)
      return;
    if (!asSceneWithHud->hudGui)
      return;

    static bool showHud = false;

    ImVec2 hudButtonPosition(cursorScene3dTopLeft.x + 2.f, cursorScene3dTopLeft.y + 2.f);
    ImVec2 hudWindowPosition(cursorScene3dTopLeft.x + 2.f, cursorScene3dTopLeft.y + 30.f);
    if (ButtonInOverlayWindow(ICON_FA_COG, hudButtonPosition, ImVec2(30.f, 30.f)))
      showHud = !showHud;

    if (showHud) {
      ImGui::SetNextWindowPos(hudWindowPosition, ImGuiCond_Once);
      ImGui::SetNextWindowBgAlpha(0.5f);
      ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
      ImGui::Begin("HUD", &showHud, flags);
      asSceneWithHud->hudGui();
      ImGui::End();
    }
  }

  void render3d()
  {
    ImVec2 sceneSize = ImGui::GetCurrentWindow()->Size;
    sceneSize.y -= 35.f;
    sceneSize.x = (float)((int)((sceneSize.x) / 4) * 4);
    ImVec2 cursorPosBeforeScene3d = ImGui::GetCursorScreenPos();
    _appContext._sceneWidget->render(sceneSize);
    renderHud(cursorPosBeforeScene3d);
  }

  void renderPlayground()
  {
    ImGui::Button(ICON_FA_QUESTION_CIRCLE);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(
        "Playground : you can edit the code below! As soon as you save it, the code will be "
        "compiled and the 3D scene "
        "will be updated");
    ImGui::SameLine();

    if (_appContext._isCompiling) {
      ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Compiling");
      _studioLayout.setVisible(DockableWindowId::Logs, true);
    }
    if (ImGui::Button(ICON_FA_PLAY " Run"))
      _playgroundCodeEditor.saveAll();
    ImGui::SameLine();
    _playgroundCodeEditor.render();
  }

private:
  void handleLoopSamples()
  {
    if (!_appContext._loopSamples.flagLoop)
      return;

    static int frame_counter = 0;
    const int max_frames     = 60;

    if (frame_counter > max_frames) {
      std::string sampleName
        = _appContext._loopSamples.samplesToLoop[_appContext._loopSamples.currentIdx];
      BABYLON_LOG_ERROR("LoopSample", sampleName)
      auto scene = SamplesInfo::SamplesCollection::Instance().createRenderableScene(sampleName, nullptr);
      this->setRenderableScene(scene);

      if (_appContext._loopSamples.currentIdx < _appContext._loopSamples.samplesToLoop.size() - 2)
        _appContext._loopSamples.currentIdx++;
      else
        _appContext._loopSamples.flagLoop = false;

      frame_counter = 0;
    }
    else
      frame_counter++;
  }

  //
  // BabylonStudioContext
  //
  struct AppContext {
    std::unique_ptr<BABYLON::ImGuiSceneWidget> _sceneWidget;
    std::unique_ptr<BABYLON::Inspector> _inspector;
    int _frameCounter = 0;
    BabylonStudioOptions _options;
    bool _isCompiling = false;
    bool _imgui_show_demo_window = false;

    BABYLON::SamplesBrowser& sampleListComponent() {
      if (!_sampleListComponent) {
        _sampleListComponent = std::make_unique<BABYLON::SamplesBrowser>();
      }
      return *_sampleListComponent;
    }

    struct {
      bool flagLoop     = false;
      size_t currentIdx = 0;
      std::vector<std::string> samplesToLoop;
    } _loopSamples;

  private:
    std::unique_ptr<BABYLON::SamplesBrowser> _sampleListComponent;
  };

  AppContext _appContext;
  BabylonStudioLayout _studioLayout;
  ImGuiUtils::CodeEditor _samplesCodeEditor
    = ImGuiUtils::CodeEditor(true); // true <-> showCheckboxReadOnly
  ImGuiUtils::CodeEditor _playgroundCodeEditor;
}; // end of class BabylonInspectorApp

// public API
#ifdef __EMSCRIPTEN__
BABYLON::BabylonStudioApp app;
#endif
void runBabylonStudio(const std::shared_ptr<BABYLON::IRenderableScene>& scene,
                      BabylonStudioOptions options /* = SceneWithInspectorOptions() */
)
{
#ifndef __EMSCRIPTEN__
  BABYLON::BabylonStudioApp app;
#endif
  std::shared_ptr<BABYLON::IRenderableScene> sceneNotNull =
    scene ? scene : std::make_shared<EmptyScene>();
  app.RunApp(sceneNotNull, options);
}

void BABYLON_SHARED_EXPORT listSamples()
{
  auto& samplesCollection = SamplesInfo::SamplesCollection::Instance();
  const auto& allSamples  = samplesCollection.AllSamples();
  for (size_t i = 0; i < allSamples.size(); ++i) {
    const auto& sampleData = allSamples[i];
    std::cout << sampleData.categoryName << "/" << sampleData.sampleName << std::endl;
  }

}

} // namespace BABYLON
