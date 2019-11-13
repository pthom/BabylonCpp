#include <babylon/postprocesses/image_processing_post_process.h>

#include <babylon/cameras/camera.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/materials/color_curves.h>
#include <babylon/materials/iimage_processing_configuration_defines.h>
#include <babylon/materials/image_processing_configuration.h>

namespace BABYLON {

ImageProcessingPostProcess::ImageProcessingPostProcess(
  const std::string& iName, float renderRatio, const CameraPtr& camera,
  unsigned int samplingMode, Engine* engine, bool reusable,
  unsigned int textureType,
  ImageProcessingConfiguration* imageProcessingConfiguration)
    : PostProcess{iName,       "imageProcessing", {},     {},       renderRatio,
                  camera,      samplingMode,      engine, reusable, "",
                  textureType, "postprocess",     {},     true}
    , imageProcessingConfiguration{this,
                                   &ImageProcessingPostProcess::
                                     get_imageProcessingConfiguration,
                                   &ImageProcessingPostProcess::
                                     set_imageProcessingConfiguration}
    , colorCurves{this, &ImageProcessingPostProcess::get_colorCurves,
                  &ImageProcessingPostProcess::set_colorCurves}
    , colorCurvesEnabled{this,
                         &ImageProcessingPostProcess::get_colorCurvesEnabled,
                         &ImageProcessingPostProcess::set_colorCurvesEnabled}
    , colorGradingTexture{this,
                          &ImageProcessingPostProcess::get_colorGradingTexture,
                          &ImageProcessingPostProcess::set_colorGradingTexture}
    , colorGradingEnabled{this,
                          &ImageProcessingPostProcess::get_colorGradingEnabled,
                          &ImageProcessingPostProcess::set_colorGradingEnabled}
    , exposure{this, &ImageProcessingPostProcess::get_exposure,
               &ImageProcessingPostProcess::set_exposure}
    , toneMappingEnabled{this,
                         &ImageProcessingPostProcess::get_toneMappingEnabled,
                         &ImageProcessingPostProcess::set_toneMappingEnabled}
    , toneMappingType{this, &ImageProcessingPostProcess::get_toneMappingType,
                      &ImageProcessingPostProcess::set_toneMappingType}
    , contrast{this, &ImageProcessingPostProcess::get_contrast,
               &ImageProcessingPostProcess::set_contrast}
    , vignetteStretch{this, &ImageProcessingPostProcess::get_vignetteStretch,
                      &ImageProcessingPostProcess::set_vignetteStretch}
    , vignetteCentreX{this, &ImageProcessingPostProcess::get_vignetteCentreX,
                      &ImageProcessingPostProcess::set_vignetteCentreX}
    , vignetteCentreY{this, &ImageProcessingPostProcess::get_vignetteCentreY,
                      &ImageProcessingPostProcess::set_vignetteCentreY}
    , vignetteWeight{this, &ImageProcessingPostProcess::get_vignetteWeight,
                     &ImageProcessingPostProcess::set_vignetteWeight}
    , vignetteColor{this, &ImageProcessingPostProcess::get_vignetteColor,
                    &ImageProcessingPostProcess::set_vignetteColor}
    , vignetteCameraFov{this,
                        &ImageProcessingPostProcess::get_vignetteCameraFov,
                        &ImageProcessingPostProcess::set_vignetteCameraFov}
    , vignetteBlendMode{this,
                        &ImageProcessingPostProcess::get_vignetteBlendMode,
                        &ImageProcessingPostProcess::set_vignetteBlendMode}
    , vignetteEnabled{this, &ImageProcessingPostProcess::get_vignetteEnabled,
                      &ImageProcessingPostProcess::set_vignetteEnabled}
    , fromLinearSpace{this, &ImageProcessingPostProcess::get_fromLinearSpace,
                      &ImageProcessingPostProcess::set_fromLinearSpace}
    , _imageProcessingObserver{nullptr}
    , _fromLinearSpace{false}
{
  // Setup the configuration as forced by the constructor. This would then not
  // force the scene materials output in linear space and let untouched the
  // default forward pass.
  if (imageProcessingConfiguration) {
    imageProcessingConfiguration->applyByPostProcess = true;
    _attachImageProcessingConfiguration(imageProcessingConfiguration, true);
    // This will cause the shader to be compiled
    fromLinearSpace = false;
  }
  // Setup the default processing configuration to the scene.
  else {
    _attachImageProcessingConfiguration(nullptr, true);

    _imageProcessingConfiguration->applyByPostProcess = true;
  }

  onApply = [&](Effect* effect, EventState& /*es*/) {
    _imageProcessingConfiguration->bind(effect, aspectRatio());
  };
}

ImageProcessingPostProcess::~ImageProcessingPostProcess() = default;

const std::string ImageProcessingPostProcess::getClassName() const
{
  return "ImageProcessingPostProcess";
}

ImageProcessingConfiguration*&
ImageProcessingPostProcess::get_imageProcessingConfiguration()
{
  return _imageProcessingConfiguration;
}

void ImageProcessingPostProcess::set_imageProcessingConfiguration(
  ImageProcessingConfiguration* const& value)
{
  // We are almost sure it is applied by post process as
  // We are in the post process :-)
  value->applyByPostProcess = true;
  _attachImageProcessingConfiguration(value);
}

void ImageProcessingPostProcess::_attachImageProcessingConfiguration(
  ImageProcessingConfiguration* configuration, bool doNotBuild)
{
  if (configuration == _imageProcessingConfiguration) {
    return;
  }

  // Detaches observer.
  if (_imageProcessingConfiguration && _imageProcessingObserver) {
    _imageProcessingConfiguration->onUpdateParameters.remove(
      _imageProcessingObserver);
  }

  // Pick the scene configuration if needed.
  if (!configuration) {
    Scene* scene = nullptr;
    auto engine  = getEngine();
    auto camera  = getCamera();

    if (camera) {
      scene = camera->getScene();
    }
    else if (engine && !engine->scenes.empty()) {
      auto& scenes = engine->scenes;
      scene        = scenes.back();
    }
    else {
      scene = Engine::LastCreatedScene();
    }

    _imageProcessingConfiguration = scene->imageProcessingConfiguration().get();
  }
  else {
    _imageProcessingConfiguration = configuration;
  }

  // Attaches observer.
  if (_imageProcessingConfiguration) {
    _imageProcessingObserver
      = _imageProcessingConfiguration->onUpdateParameters.add(
        [this](ImageProcessingConfiguration* /*conf*/, EventState& /*es*/) {
          _updateParameters();
        });
  }

  // Ensure the effect will be rebuilt.
  if (!doNotBuild) {
    _updateParameters();
  }
}

std::shared_ptr<ColorCurves>& ImageProcessingPostProcess::get_colorCurves()
{
  return _imageProcessingConfiguration->colorCurves;
}

void ImageProcessingPostProcess::set_colorCurves(
  const std::shared_ptr<ColorCurves>& value)
{
  _imageProcessingConfiguration->colorCurves = value;
}

bool ImageProcessingPostProcess::get_colorCurvesEnabled() const
{
  return _imageProcessingConfiguration->colorCurvesEnabled();
}

void ImageProcessingPostProcess::set_colorCurvesEnabled(bool value)
{
  _imageProcessingConfiguration->colorCurvesEnabled = value;
}

BaseTexturePtr& ImageProcessingPostProcess::get_colorGradingTexture()
{
  return _imageProcessingConfiguration->colorGradingTexture;
}

void ImageProcessingPostProcess::set_colorGradingTexture(
  const BaseTexturePtr& value)
{
  _imageProcessingConfiguration->colorGradingTexture = value;
}

bool ImageProcessingPostProcess::get_colorGradingEnabled() const
{
  return _imageProcessingConfiguration->colorGradingEnabled();
}

void ImageProcessingPostProcess::set_colorGradingEnabled(bool value)
{
  _imageProcessingConfiguration->colorGradingEnabled = value;
}

float ImageProcessingPostProcess::get_exposure() const
{
  return _imageProcessingConfiguration->exposure();
}

void ImageProcessingPostProcess::set_exposure(float value)
{
  _imageProcessingConfiguration->exposure = value;
}

bool ImageProcessingPostProcess::get_toneMappingEnabled() const
{
  return _imageProcessingConfiguration->toneMappingEnabled();
}

void ImageProcessingPostProcess::set_toneMappingEnabled(bool value)
{
  _imageProcessingConfiguration->toneMappingEnabled = value;
}

unsigned int ImageProcessingPostProcess::get_toneMappingType() const
{
  return _imageProcessingConfiguration->toneMappingType();
}

void ImageProcessingPostProcess::set_toneMappingType(unsigned int value)
{
  _imageProcessingConfiguration->toneMappingType = value;
}

float ImageProcessingPostProcess::get_contrast() const
{
  return _imageProcessingConfiguration->contrast();
}

void ImageProcessingPostProcess::set_contrast(float value)
{
  _imageProcessingConfiguration->contrast = value;
}

float ImageProcessingPostProcess::get_vignetteStretch() const
{
  return _imageProcessingConfiguration->vignetteStretch;
}

void ImageProcessingPostProcess::set_vignetteStretch(float value)
{
  _imageProcessingConfiguration->vignetteStretch = value;
}

float ImageProcessingPostProcess::get_vignetteCentreX() const
{
  return _imageProcessingConfiguration->vignetteCentreX;
}

void ImageProcessingPostProcess::set_vignetteCentreX(float value)
{
  _imageProcessingConfiguration->vignetteCentreX = value;
}

float ImageProcessingPostProcess::get_vignetteCentreY() const
{
  return _imageProcessingConfiguration->vignetteCentreY;
}

void ImageProcessingPostProcess::set_vignetteCentreY(float value)
{
  _imageProcessingConfiguration->vignetteCentreY = value;
}

float ImageProcessingPostProcess::get_vignetteWeight() const
{
  return _imageProcessingConfiguration->vignetteWeight;
}

void ImageProcessingPostProcess::set_vignetteWeight(float value)
{
  _imageProcessingConfiguration->vignetteWeight = value;
}

Color4& ImageProcessingPostProcess::get_vignetteColor()
{
  return _imageProcessingConfiguration->vignetteColor;
}

void ImageProcessingPostProcess::set_vignetteColor(const Color4& value)
{
  _imageProcessingConfiguration->vignetteColor = value;
}

float ImageProcessingPostProcess::get_vignetteCameraFov() const
{
  return _imageProcessingConfiguration->vignetteCameraFov;
}

void ImageProcessingPostProcess::set_vignetteCameraFov(float value)
{
  _imageProcessingConfiguration->vignetteCameraFov = value;
}

unsigned int ImageProcessingPostProcess::get_vignetteBlendMode() const
{
  return _imageProcessingConfiguration->vignetteBlendMode();
}

void ImageProcessingPostProcess::set_vignetteBlendMode(unsigned int value)
{
  _imageProcessingConfiguration->vignetteBlendMode = value;
}

bool ImageProcessingPostProcess::get_vignetteEnabled() const
{
  return _imageProcessingConfiguration->vignetteEnabled();
}

void ImageProcessingPostProcess::set_vignetteEnabled(bool value)
{
  _imageProcessingConfiguration->vignetteEnabled = value;
}

bool ImageProcessingPostProcess::get_fromLinearSpace() const
{
  return _fromLinearSpace;
}

void ImageProcessingPostProcess::set_fromLinearSpace(bool value)
{
  if (_fromLinearSpace == value) {
    return;
  }

  _fromLinearSpace = value;
  _updateParameters();
}

void ImageProcessingPostProcess::_updateParameters()
{
  _defines.FROMLINEARSPACE = _fromLinearSpace;
  _imageProcessingConfiguration->prepareDefines(_defines, true);

  std::string defines = "";
  if (_defines.IMAGEPROCESSING) {
    defines += "#define IMAGEPROCESSING;\r\n";
  }
  else if (_defines.VIGNETTE) {
    defines += "#define VIGNETTE;\r\n";
  }
  else if (_defines.VIGNETTEBLENDMODEMULTIPLY) {
    defines += "#define VIGNETTEBLENDMODEMULTIPLY;\r\n";
  }
  else if (_defines.VIGNETTEBLENDMODEOPAQUE) {
    defines += "#define VIGNETTEBLENDMODEOPAQUE;\r\n";
  }
  else if (_defines.TONEMAPPING) {
    defines += "#define TONEMAPPING;\r\n";
  }
  else if (_defines.CONTRAST) {
    defines += "#define CONTRAST;\r\n";
  }
  else if (_defines.EXPOSURE) {
    defines += "#define EXPOSURE;\r\n";
  }
  else if (_defines.COLORCURVES) {
    defines += "#define COLORCURVES;\r\n";
  }
  else if (_defines.COLORGRADING) {
    defines += "#define COLORGRADING;\r\n";
  }
  else if (_defines.FROMLINEARSPACE) {
    defines += "#define FROMLINEARSPACE;\r\n";
  }
  else if (_defines.SAMPLER3DGREENDEPTH) {
    defines += "#define SAMPLER3DGREENDEPTH;\r\n";
  }
  else if (_defines.SAMPLER3DBGRMAP) {
    defines += "#define SAMPLER3DBGRMAP;\r\n";
  }
  else if (_defines.IMAGEPROCESSINGPOSTPROCESS) {
    defines += "#define IMAGEPROCESSINGPOSTPROCESS;\r\n";
  }
  else if (_defines.EXPOSURE) {
    defines += "#define EXPOSURE;\r\n";
  }

  std::vector<std::string> samplers{"textureSampler"};
  ImageProcessingConfiguration::PrepareSamplers(samplers, _defines);

  std::vector<std::string> uniforms{"scale"};

  ImageProcessingConfiguration::PrepareSamplers(samplers, _defines);
  ImageProcessingConfiguration::PrepareUniforms(uniforms, _defines);

  updateEffect(defines, uniforms, samplers);
}

void ImageProcessingPostProcess::dispose(Camera* camera)
{
  PostProcess::dispose(camera);

  if (_imageProcessingConfiguration && _imageProcessingObserver) {
    _imageProcessingConfiguration->onUpdateParameters.remove(
      _imageProcessingObserver);
  }

  if (_imageProcessingConfiguration) {
    _imageProcessingConfiguration->applyByPostProcess = false;
  }
}

} // end of namespace BABYLON
