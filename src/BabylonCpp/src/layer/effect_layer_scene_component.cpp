#include <babylon/layer/effect_layer_scene_component.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/cameras/camera.h>
#include <babylon/engines/abstract_scene.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/layer/effect_layer.h>
#include <babylon/materials/textures/render_target_texture.h>
#include <babylon/meshes/abstract_mesh.h>

namespace BABYLON {

EffectLayerSceneComponent::EffectLayerSceneComponent(Scene* iScene)
    : _renderEffects{false}, _needStencil{false}, _previousStencilState{false}
{
  ISceneComponent::name = EffectLayerSceneComponent::name;
  scene                 = iScene;
  _engine               = scene->getEngine();
}

EffectLayerSceneComponent::~EffectLayerSceneComponent() = default;

void EffectLayerSceneComponent::_register()
{
  scene->_isReadyForMeshStage.registerStep(
    SceneComponentConstants::STEP_ISREADYFORMESH_EFFECTLAYER, this,
    [this](AbstractMesh* mesh, bool hardwareInstancedRendering) {
      return _isReadyForMesh(mesh, hardwareInstancedRendering);
    });

  scene->_cameraDrawRenderTargetStage.registerStep(
    SceneComponentConstants::STEP_CAMERADRAWRENDERTARGET_EFFECTLAYER, this,
    [this](Camera* camera) -> bool {
      _renderMainTexture(camera);
      return true;
    });

  scene->_beforeCameraDrawStage.registerStep(
    SceneComponentConstants::STEP_BEFORECAMERADRAW_EFFECTLAYER, this,
    [this](Camera * /*camera*/) -> bool {
      _setStencil();
      return true;
    });

  scene->_afterRenderingGroupDrawStage.registerStep(
    SceneComponentConstants::STEP_AFTERRENDERINGGROUPDRAW_EFFECTLAYER_DRAW,
    this,
    [this](int renderingGroupId) { _drawRenderingGroup(renderingGroupId); });

  scene->_afterCameraDrawStage.registerStep(
    SceneComponentConstants::STEP_AFTERCAMERADRAW_EFFECTLAYER, this,
    [this](Camera * /*camera*/) -> bool {
      _setStencilBack();
      return true;
    });
  scene->_afterCameraDrawStage.registerStep(
    SceneComponentConstants::STEP_AFTERCAMERADRAW_EFFECTLAYER_DRAW, this,
    [this](Camera * /*camera*/) -> bool {
      _drawCamera();
      return true;
    });
}

void EffectLayerSceneComponent::rebuild()
{
  const auto& layers = scene->effectLayers;
  for (const auto& effectLayer : layers) {
    effectLayer->_rebuild();
  }
}

void EffectLayerSceneComponent::serialize(json& /*serializationObject*/) const
{
}

void EffectLayerSceneComponent::addFromContainer(AbstractScene* container)
{
  if (container->effectLayers.empty()) {
    return;
  }
  for (const auto& o : container->effectLayers) {
    scene->addEffectLayer(o);
  }
}

void EffectLayerSceneComponent::removeFromContainer(AbstractScene* container,
                                                    bool dispose)
{
  if (container->effectLayers.empty()) {
    return;
  }
  for (const auto& o : container->effectLayers) {
    scene->removeEffectLayer(o);
    if (dispose) {
      o->dispose();
    }
  }
}

void EffectLayerSceneComponent::dispose()
{
  auto& layers = scene->effectLayers;
  for (const auto& layer : layers) {
    layer->dispose();
  }
  layers.clear();
}

bool EffectLayerSceneComponent::_isReadyForMesh(AbstractMesh* mesh,
                                                bool hardwareInstancedRendering)
{
  const auto& layers = scene->effectLayers;
  for (const auto& layer : layers) {
    if (!layer->hasMesh(mesh)) {
      continue;
    }

    for (const auto& subMesh : mesh->subMeshes) {
      if (!layer->isReady(subMesh.get(), hardwareInstancedRendering)) {
        return false;
      }
    }
  }
  return true;
}

bool EffectLayerSceneComponent::_renderMainTexture(Camera* camera)
{
  _renderEffects = false;
  _needStencil   = false;

  auto needRebind = false;

  auto& layers = scene->effectLayers;
  if (!layers.empty()) {
    _previousStencilState = _engine->getStencilBuffer();
    for (const auto& effectLayer : layers) {
      if (effectLayer->shouldRender()
          && (!effectLayer->camera()
              || (effectLayer->camera()->cameraRigMode == Camera::RIG_MODE_NONE
                  && camera == effectLayer->camera().get())
              || (effectLayer->camera()->cameraRigMode != Camera::RIG_MODE_NONE
                  && stl_util::contains(effectLayer->camera()->_rigCameras,
                                        camera)))) {

        _renderEffects = true;
        _needStencil   = _needStencil || effectLayer->needStencil();

        auto& renderTarget = effectLayer->_mainTexture;
        if (renderTarget->_shouldRender()) {
          scene->incrementRenderId();
          renderTarget->render(false, false);
          needRebind = true;
        }
      }
    }

    scene->incrementRenderId();
  }

  return needRebind;
}

void EffectLayerSceneComponent::_setStencil()
{
  // Activate effect Layer stencil
  if (_needStencil) {
    _engine->setStencilBuffer(true);
  }
}

void EffectLayerSceneComponent::_setStencilBack()
{
  // Restore effect Layer stencil
  if (_needStencil) {
    _engine->setStencilBuffer(_previousStencilState);
  }
}

void EffectLayerSceneComponent::_draw(int renderingGroupId)
{
  if (_renderEffects) {
    _engine->setDepthBuffer(false);

    auto& layers = scene->effectLayers;
    for (const auto& effectLayer : layers) {
      if (effectLayer->renderingGroupId() == renderingGroupId) {
        if (effectLayer->shouldRender()) {
          effectLayer->render();
        }
      }
    }
    _engine->setDepthBuffer(true);
  }
}

void EffectLayerSceneComponent::_drawCamera()
{
  if (_renderEffects) {
    _draw(-1);
  }
}

void EffectLayerSceneComponent::_drawRenderingGroup(int index)
{
  if (!scene->_isInIntermediateRendering() && _renderEffects) {
    _draw(index);
  }
}

} // end of namespace BABYLON
