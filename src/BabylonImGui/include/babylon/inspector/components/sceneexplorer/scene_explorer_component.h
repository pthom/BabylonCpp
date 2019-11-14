#ifndef BABYLON_INSPECTOR_COMPONENTS_SCENE_EXPLORER_SCENE_EXPLORER_COMPONENT_H
#define BABYLON_INSPECTOR_COMPONENTS_SCENE_EXPLORER_SCENE_EXPLORER_COMPONENT_H

#include <memory>

#include <babylon/babylon_api.h>

namespace BABYLON {

class GlobalState;
class Scene;
using GlobalStatePtr = std::shared_ptr<GlobalState>;
class SceneGraphComponent;

struct ISceneExplorerComponentProps {
  Scene* scene               = nullptr;
  GlobalStatePtr globalState = nullptr;
}; // end of struct ISceneExplorerComponentProps

class BABYLON_SHARED_EXPORT SceneExplorerComponent {

public:
  SceneExplorerComponent(const ISceneExplorerComponentProps& props);
  ~SceneExplorerComponent(); // = default

  void render();

private:
  ISceneExplorerComponentProps props;
  std::unique_ptr<SceneGraphComponent> _sceneGraphComponent;

}; // end of class SceneExplorerComponent

} // end of namespace BABYLON

#endif // end of
       // BABYLON_INSPECTOR_COMPONENTS_SCENE_EXPLORER_SCENE_EXPLORER_COMPONENT_H
