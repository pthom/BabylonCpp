#ifndef BABYLON_LAYER_EFFECT_LAYER_SCENE_COMPONENT_H
#define BABYLON_LAYER_EFFECT_LAYER_SCENE_COMPONENT_H

#include <map>
#include <nlohmann/json_fwd.hpp>
#include <vector>

#include <babylon/babylon_api.h>
#include <babylon/engines/iscene_component.h>
#include <babylon/engines/scene_component_constants.h>

using json = nlohmann::json;

namespace BABYLON {

class AbstractMesh;
class AbstractScene;
class Camera;
class EffectLayerSceneComponent;
class Engine;
using EffectLayerSceneComponentPtr = std::shared_ptr<EffectLayerSceneComponent>;

/**
 * @brief Defines the layer scene component responsible to manage any effect
 * layers in a given scene.
 */
class BABYLON_SHARED_EXPORT EffectLayerSceneComponent : public ISceneComponent {

public:
  /**
   * The component name helpfull to identify the component in the list of scene
   * components.
   */
  static constexpr const char* name = SceneComponentConstants::NAME_EFFECTLAYER;

public:
  template <typename... Ts>
  static EffectLayerSceneComponentPtr New(Ts&&... args)
  {
    return std::shared_ptr<EffectLayerSceneComponent>(
      new EffectLayerSceneComponent(std::forward<Ts>(args)...));
  }
  virtual ~EffectLayerSceneComponent(); // = default

  /**
   * @brief Registers the component in a given scene.
   */
  void _register() override;

  /**
   * @brief Rebuilds the elements related to this component in case of
   * context lost for instance.
   */
  void rebuild() override;

  /**
   * @brief Serializes the component data to the specified json object
   * @param serializationObject The object to serialize to
   */
  void serialize(json& serializationObject) const;

  /**
   * @brief Adds all the elements from the container to the scene.
   * @param container the container holding the elements
   */
  void addFromContainer(AbstractScene* container);

  /**
   * @brief Removes all the elements in the container from the scene
   * @param container contains the elements to remove
   * @param dispose if the removed element should be disposed (default: false)
   */
  void removeFromContainer(AbstractScene* container, bool dispose = false);

  /**
   * @brief Disposes the component and the associated resources.
   */
  void dispose() override;

protected:
  /**
   * @brief Creates a new instance of the component for the given scene.
   * @param scene Defines the scene to register the component in
   */
  EffectLayerSceneComponent(Scene* scene);

private:
  bool _isReadyForMesh(AbstractMesh* mesh, bool hardwareInstancedRendering);
  bool _renderMainTexture(Camera* camera);
  void _setStencil();
  void _setStencilBack();
  void _draw(int renderingGroupId);
  void _drawCamera();
  void _drawRenderingGroup(int index);

private:
  Engine* _engine;
  bool _renderEffects;
  bool _needStencil;
  bool _previousStencilState;

}; // end of class EffectLayerSceneComponent

} // end of namespace BABYLON

#endif // end of BABYLON_RENDERING_BOUNDING_BOX_RENDERER_H
