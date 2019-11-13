#include <babylon/postprocesses/renderpipeline/post_process_render_pipeline.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/cameras/camera.h>
#include <babylon/engines/engine.h>
#include <babylon/postprocesses/display_pass_post_process.h>
#include <babylon/postprocesses/renderpipeline/post_process_render_effect.h>

namespace BABYLON {

PostProcessRenderPipeline::PostProcessRenderPipeline(Engine* iEngine,
                                                     const std::string& name)

    : _name{name}
    , name{this, &PostProcessRenderPipeline::get_name}
    , isSupported{this, &PostProcessRenderPipeline::get_isSupported}
    , engine{iEngine}
{
}

PostProcessRenderPipeline::~PostProcessRenderPipeline() = default;

const std::string PostProcessRenderPipeline::getClassName() const
{
  return "PostProcessRenderPipeline";
}

std::string PostProcessRenderPipeline::get_name() const
{
  return _name;
}

bool PostProcessRenderPipeline::get_isSupported() const
{
  for (const auto& item : _renderEffects) {
    if (!item.second->isSupported()) {
      return false;
    }
  }
  return true;
}

void PostProcessRenderPipeline::addEffect(
  const PostProcessRenderEffectPtr& renderEffect)
{
  _renderEffects[renderEffect->_name] = renderEffect;
}

void PostProcessRenderPipeline::_rebuild()
{
}

void PostProcessRenderPipeline::_enableEffect(
  const std::string& renderEffectName, const std::vector<CameraPtr>& cameras)
{
  auto& renderEffects = _renderEffects[renderEffectName];

  if (!renderEffects) {
    return;
  }

  auto& _cam = cameras.empty() ? _cameras : cameras;
  renderEffects->_enable(_cam);
}

void PostProcessRenderPipeline::_disableEffect(
  const std::string& renderEffectName, const std::vector<CameraPtr>& cameras)
{
  if (!stl_util::contains(_renderEffects, renderEffectName)) {
    return;
  }

  auto& _cam = cameras.empty() ? _cameras : cameras;
  _renderEffects[renderEffectName]->_disable(_cam);
}

void PostProcessRenderPipeline::_attachCameras(
  const std::vector<CameraPtr>& cameras, bool unique)
{
  auto& cams = cameras.empty() ? _cameras : cameras;

  if (cams.empty()) {
    return;
  }

  std::vector<CameraPtr> camerasToDelete;
  for (const auto& camera : cams) {
    const auto& cameraName = camera->name;

    auto it = std::find_if(_cameras.begin(), _cameras.end(),
                           [&cameraName](const CameraPtr& cameraEntry) {
                             return cameraName == cameraEntry->name;
                           });
    if (it == _cameras.end()) {
      _cameras.emplace_back(camera);
    }
    else if (unique) {
      camerasToDelete.emplace_back(camera);
    }
  }

  for (const auto& camera : camerasToDelete) {
    auto it = std::find_if(_cameras.begin(), _cameras.end(),
                           [&camera](const CameraPtr& cameraEntry) {
                             return camera->name == cameraEntry->name;
                           });
    if (it != _cameras.end())
      _cameras.erase(it);
  }

  for (const auto& item : _renderEffects) {
    item.second->_attachCameras(cams);
  }
}

void PostProcessRenderPipeline::_detachCameras(
  const std::vector<CameraPtr>& cameras)
{
  auto cams = cameras.empty() ? _cameras : cameras;

  if (cams.empty()) {
    return;
  }

  for (const auto& item : _renderEffects) {
    item.second->_detachCameras(cams);
  }

  for (const auto& camera : cams) {
    auto it = std::find_if(_cameras.begin(), _cameras.end(),
                           [&camera](const CameraPtr& cameraEntry) {
                             return camera->name == cameraEntry->name;
                           });
    if (it != _cameras.end()) {
      _cameras.erase(it);
    }
  }
}

void PostProcessRenderPipeline::_update()
{
  for (const auto& item : _renderEffects) {
    item.second->_update();
  }

  for (const auto& camera : _cameras) {
    const auto& cameraName = camera->name;
    if (stl_util::contains(_renderEffectsForIsolatedPass, cameraName)) {
      _renderEffectsForIsolatedPass[cameraName]->_update();
    }
  }
}

void PostProcessRenderPipeline::_reset()
{
  _renderEffects.clear();
  _renderEffectsForIsolatedPass.clear();
}

bool PostProcessRenderPipeline::_enableMSAAOnFirstPostProcess(
  unsigned int sampleCount)
{
  if (engine->webGLVersion() == 1.f) {
    return false;
  }

  // Set samples of the very first post process to 4 to enable native
  // anti-aliasing in browsers that support webGL 2.0 (See:
  // https://github.com/BabylonJS/Babylon.js/issues/3754)
  auto effectKeys = stl_util::extract_keys(_renderEffects);

  if (!effectKeys.empty()) {
    auto postProcesses = _renderEffects[effectKeys[0]]->getPostProcesses();
    if (!postProcesses.empty()) {
      postProcesses[0]->samples = sampleCount;
      return true;
    }
  }
  return false;
}

void PostProcessRenderPipeline::dispose(bool /*doNotRecurse*/,
                                        bool /*disposeMaterialAndTextures*/)
{
  // Must be implemented by children
}

} // end of namespace BABYLON
