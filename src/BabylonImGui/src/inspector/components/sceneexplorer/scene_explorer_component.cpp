#include <babylon/inspector/components/sceneexplorer/scene_explorer_component.h>

#include <string>
#include <vector>

// ImGui
#include <imgui.h>

#include <babylon/engines/scene.h>
#include <babylon/inspector/components/sceneexplorer/scene_graph_component.h>

namespace BABYLON {

SceneExplorerComponent::SceneExplorerComponent(
  const ISceneExplorerComponentProps& iProps)
    : props{iProps}
{
  SceneGraphComponentProps compProps;
  compProps.scene       = iProps.scene;
  compProps.globalState = iProps.globalState;
  _sceneGraphComponent  = std::make_unique<SceneGraphComponent>(compProps);
}

SceneExplorerComponent::~SceneExplorerComponent() = default;

void SceneExplorerComponent::render()
{
  // Filter
  {
    ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() - 120.f);
    static ImGuiTextFilter filter;
    filter.Draw();
    const std::vector<std::string> lines;
    for (const auto& line : lines) {
      if (filter.PassFilter(line.c_str())) {
        ImGui::BulletText("%s", line.c_str());
      }
    }
  }

  ImGui::Separator();

  // The Scene Graph
  if (_sceneGraphComponent) {
    _sceneGraphComponent->render();
  }
}

} // end of namespace BABYLON
