#include <babylon/helpers/environment_helper.h>

#include <babylon/cameras/arc_rotate_camera.h>
#include <babylon/cameras/camera.h>
#include <babylon/core/string.h>
#include <babylon/engines/scene.h>
#include <babylon/helpers/iscene_size.h>
#include <babylon/materials/background/background_material.h>
#include <babylon/materials/image_processing_configuration.h>
#include <babylon/materials/textures/cube_texture.h>
#include <babylon/materials/textures/mirror_texture.h>
#include <babylon/materials/textures/render_target_texture.h>
#include <babylon/materials/textures/texture_constants.h>
#include <babylon/meshes/mesh.h>

namespace BABYLON {

constexpr const char* EnvironmentHelper::_groundTextureCDNUrl;
constexpr const char* EnvironmentHelper::_skyboxTextureCDNUrl;
constexpr const char* EnvironmentHelper::_environmentTextureCDNUrl;

EnvironmentHelper::EnvironmentHelper(Scene* scene)
    : EnvironmentHelper(EnvironmentHelper::_getDefaultOptions(), scene)

{
}

EnvironmentHelper::EnvironmentHelper(
  const std::optional<IEnvironmentHelperOptions>& options, Scene* scene)
    : _rootMesh{nullptr}
    , _skybox{nullptr}
    , _skyboxTexture{nullptr}
    , _skyboxMaterial{nullptr}
    , _ground{nullptr}
    , _groundTexture{nullptr}
    , _groundMirror{nullptr}
    , _groundMaterial{nullptr}
    , _scene{scene}
    , _options{options.has_value() ? *options :
                                     EnvironmentHelper::_getDefaultOptions()}
{
  _setupBackground();
  _setupImageProcessing();
}

EnvironmentHelper::~EnvironmentHelper() = default;

void EnvironmentHelper::updateOptions(const IEnvironmentHelperOptions& options)
{
  auto& newOptions = options;

  if (_ground && !newOptions.createGround) {
    _ground->dispose();
    _ground = nullptr;
  }

  if (_groundMaterial && !newOptions.createGround) {
    _groundMaterial->dispose();
    _groundMaterial = nullptr;
  }

  if (_groundTexture) {
    if (std::holds_alternative<BaseTexturePtr>(_options.groundTexture)
        && std::holds_alternative<BaseTexturePtr>(newOptions.groundTexture)
        && std::get<BaseTexturePtr>(_options.groundTexture)
             != std::get<BaseTexturePtr>(newOptions.groundTexture)) {
      _groundTexture->dispose();
      _groundTexture = nullptr;
    }
  }

  if (_skybox && !newOptions.createSkybox) {
    _skybox->dispose();
    _skybox = nullptr;
  }

  if (_skyboxMaterial && !newOptions.createSkybox) {
    _skyboxMaterial->dispose();
    _skyboxMaterial = nullptr;
  }

  if (_skyboxTexture) {
    if (std::holds_alternative<BaseTexturePtr>(_options.skyboxTexture)
        && std::holds_alternative<BaseTexturePtr>(newOptions.skyboxTexture)
        && std::get<BaseTexturePtr>(_options.skyboxTexture)
             != std::get<BaseTexturePtr>(newOptions.skyboxTexture)) {
      _skyboxTexture->dispose();
      _skyboxTexture = nullptr;
    }
  }

  if (_groundMirror && !newOptions.enableGroundMirror) {
    _groundMirror->dispose();
    _groundMirror = nullptr;
  }

  if (_scene->environmentTexture()) {
    if (std::holds_alternative<BaseTexturePtr>(_options.environmentTexture)
        && std::holds_alternative<BaseTexturePtr>(newOptions.environmentTexture)
        && std::get<BaseTexturePtr>(_options.environmentTexture)
             != std::get<BaseTexturePtr>(newOptions.environmentTexture)) {
      _scene->environmentTexture()->dispose();
    }
  }

  _options = newOptions;

  _setupBackground();
  _setupImageProcessing();
}

void EnvironmentHelper::setMainColor(const Color3& color)
{
  if (groundMaterial()) {
    groundMaterial()->primaryColor = color;
  }

  if (skyboxMaterial()) {
    skyboxMaterial()->primaryColor = color;
  }

  if (groundMirror()) {
    groundMirror()->clearColor = Color4(color.r, color.g, color.b, 1.f);
  }
}

MeshPtr& EnvironmentHelper::rootMesh()
{
  return _rootMesh;
}

MeshPtr& EnvironmentHelper::skybox()
{
  return _skybox;
}

BaseTexturePtr EnvironmentHelper::skyboxTexture()
{
  return _skyboxTexture;
}

BackgroundMaterialPtr& EnvironmentHelper::skyboxMaterial()
{
  return _skyboxMaterial;
}

MeshPtr& EnvironmentHelper::ground()
{
  return _ground;
}

BaseTexture* EnvironmentHelper::groundTexture()
{
  return _groundTexture;
}

MirrorTexturePtr& EnvironmentHelper::groundMirror()
{
  return _groundMirror;
}

std::vector<AbstractMesh*>& EnvironmentHelper::groundMirrorRenderList()
{
  if (_groundMirror) {
    return _groundMirror->renderList;
  }
  return _emptyGroundMirrorRenderList;
}

BackgroundMaterialPtr& EnvironmentHelper::groundMaterial()
{
  return _groundMaterial;
}

IEnvironmentHelperOptions EnvironmentHelper::_getDefaultOptions()
{
  const auto preprocessCDNUrl = [](const char* textureCDNUrl) -> std::string {
    std::string textureCDNUrlStr(textureCDNUrl);
    if (String::startsWith(textureCDNUrlStr, "https://assets.babylonjs.com")) {
      textureCDNUrlStr = String::replace(
        textureCDNUrlStr, "https://assets.babylonjs.com", "textures");
    }
    return textureCDNUrlStr;
  };

  IEnvironmentHelperOptions options;
  options.createGround  = true;
  options.groundSize    = 15;
  options.groundTexture = preprocessCDNUrl(_groundTextureCDNUrl);
  options.groundColor   = Color3(0.2f, 0.2f, 0.3f).toLinearSpace().scale(3.f);
  options.groundOpacity = 0.9f;
  options.enableGroundShadow = true;
  options.groundShadowLevel  = 0.5f;

  options.enableGroundMirror          = false;
  options.groundMirrorSizeRatio       = 0.3f;
  options.groundMirrorBlurKernel      = 64;
  options.groundMirrorAmount          = 1;
  options.groundMirrorFresnelWeight   = 1;
  options.groundMirrorFallOffDistance = 0;
  options.groundMirrorTextureType     = Constants::TEXTURETYPE_UNSIGNED_INT;

  options.groundYBias = 0.00001f;

  options.createSkybox  = true;
  options.skyboxSize    = 20;
  options.skyboxTexture = preprocessCDNUrl(_skyboxTextureCDNUrl);
  options.skyboxColor   = Color3(0.2f, 0.2f, 0.3f).toLinearSpace().scale(3.f);

  options.backgroundYRotation = 0.f;
  options.sizeAuto            = true;
  options.rootPosition        = Vector3::Zero();

  options.setupImageProcessing = true;
  options.environmentTexture   = preprocessCDNUrl(_environmentTextureCDNUrl);
  options.cameraExposure       = 0.8f;
  options.cameraContrast       = 1.2f;
  options.toneMappingEnabled   = true;

  return options;
}

void EnvironmentHelper::_setupImageProcessing()
{
  if (_options.setupImageProcessing) {
    _scene->imageProcessingConfiguration()->contrast = _options.cameraContrast;
    _scene->imageProcessingConfiguration()->exposure = _options.cameraExposure;
    _scene->imageProcessingConfiguration()->toneMappingEnabled
      = _options.toneMappingEnabled;
    _setupEnvironmentTexture();
  }
}

void EnvironmentHelper::_setupEnvironmentTexture()
{
  if (_scene->environmentTexture()) {
    return;
  }

  if (std::holds_alternative<BaseTexturePtr>(_options.environmentTexture)) {
    _scene->environmentTexture
      = std::get<BaseTexturePtr>(_options.environmentTexture);
    return;
  }

  const auto environmentTexture = CubeTexture::CreateFromPrefilteredData(
    std::get<std::string>(_options.environmentTexture), _scene);
  _scene->environmentTexture = environmentTexture;
}

void EnvironmentHelper::_setupBackground()
{
  if (!_rootMesh) {
    _rootMesh = Mesh::New("BackgroundHelper", _scene);
  }
  _rootMesh->rotation().y = _options.backgroundYRotation;

  const auto sceneSize = _getSceneSize();
  if (_options.createGround) {
    _setupGround(sceneSize);
    _setupGroundMaterial();
    _setupGroundDiffuseTexture();

    if (_options.enableGroundMirror) {
      _setupGroundMirrorTexture(sceneSize);
    }
    _setupMirrorInGroundMaterial();
  }

  if (_options.createSkybox) {
    _setupSkybox(sceneSize);
    _setupSkyboxMaterial();
    _setupSkyboxReflectionTexture();
  }

  _rootMesh->position().x = sceneSize.rootPosition.x;
  _rootMesh->position().z = sceneSize.rootPosition.z;
  _rootMesh->position().y = sceneSize.rootPosition.y;
}

ISceneSize EnvironmentHelper::_getSceneSize()
{
  auto groundSize   = _options.groundSize;
  auto skyboxSize   = _options.skyboxSize;
  auto rootPosition = _options.rootPosition;
  if (_scene->meshes.empty() || _scene->meshes.size() == 1) {
    // 1 only means the root of the helper.
    return ISceneSize{groundSize, skyboxSize, rootPosition};
  }

  const auto sceneExtends
    = _scene->getWorldExtends([this](const AbstractMeshPtr& mesh) -> bool {
        return (mesh != _ground && mesh != _rootMesh && mesh != _skybox);
      });
  const auto sceneDiagonal = sceneExtends.max.subtract(sceneExtends.min);

  if (_options.sizeAuto) {
    if (_scene->activeCamera()->type() == Type::ARCROTATECAMERA) {
      auto activecamera
        = std::static_pointer_cast<ArcRotateCamera>(_scene->activeCamera());
      if (activecamera->upperRadiusLimit.has_value()) {
        groundSize = *activecamera->upperRadiusLimit * 2.f;
        skyboxSize = groundSize;
      }
    }

    const auto sceneDiagonalLenght = sceneDiagonal.length();
    if (sceneDiagonalLenght > groundSize) {
      groundSize = sceneDiagonalLenght * 2.f;
      skyboxSize = groundSize;
    }

    // 10 % bigger.
    groundSize *= 1.1f;
    skyboxSize *= 1.5f;
    rootPosition   = sceneExtends.min.add(sceneDiagonal.scale(0.5f));
    rootPosition.y = sceneExtends.min.y - _options.groundYBias;
  }

  return ISceneSize{groundSize, skyboxSize, rootPosition};
}

void EnvironmentHelper::_setupGround(const ISceneSize& sceneSize)
{
  if (!_ground || _ground->isDisposed()) {
    _ground
      = Mesh::CreatePlane("BackgroundPlane", sceneSize.groundSize, _scene);
    _ground->rotation().x = Math::PI_2; // Face up by default.
    _ground->setParent(_rootMesh.get());
    _ground->onDisposeObservable.add(
      [this](Node* /*node*/, EventState& /*es*/) { _ground = nullptr; });
  }

  _ground->receiveShadows = _options.enableGroundShadow;
}

void EnvironmentHelper::_setupGroundMaterial()
{
  if (!_groundMaterial) {
    _groundMaterial
      = BackgroundMaterial::New("BackgroundPlaneMaterial", _scene);
  }
  _groundMaterial->alpha        = _options.groundOpacity;
  _groundMaterial->alphaMode    = Constants::ALPHA_PREMULTIPLIED_PORTERDUFF;
  _groundMaterial->shadowLevel  = _options.groundShadowLevel;
  _groundMaterial->primaryColor = _options.groundColor;
  _groundMaterial->useRGBColor  = false;
  _groundMaterial->enableNoise  = true;

  if (_ground) {
    _ground->material = _groundMaterial;
  }
}

void EnvironmentHelper::_setupGroundDiffuseTexture()
{
  if (!_groundMaterial) {
    return;
  }

  if (_groundTexture) {
    return;
  }

  if (std::holds_alternative<BaseTexturePtr>(_options.skyboxTexture)) {
    _groundMaterial->diffuseTexture
      = std::get<BaseTexturePtr>(_options.groundTexture);
    return;
  }

  const auto diffuseTexture
    = Texture::New(std::get<std::string>(_options.groundTexture), _scene);
  diffuseTexture->gammaSpace      = false;
  diffuseTexture->hasAlpha        = true;
  _groundMaterial->diffuseTexture = diffuseTexture;
}

void EnvironmentHelper::_setupGroundMirrorTexture(const ISceneSize& sceneSize)
{
  auto wrapping = TextureConstants::CLAMP_ADDRESSMODE;
  if (!_groundMirror) {
    _groundMirror = MirrorTexture::New(
      "BackgroundPlaneMirrorTexture", _options.groundMirrorSizeRatio, _scene,
      false, _options.groundMirrorTextureType,
      TextureConstants::BILINEAR_SAMPLINGMODE, true);
    _groundMirror->mirrorPlane = Plane(0, -1, 0, sceneSize.rootPosition.y);
    _groundMirror->anisotropicFilteringLevel = 1;
    _groundMirror->wrapU                     = wrapping;
    _groundMirror->wrapV                     = wrapping;
    _groundMirror->gammaSpace                = false;

    if (!_groundMirror->renderList().empty()) {
      for (const auto& mesh : _scene->meshes) {
        if (mesh != _ground && mesh != _skybox && mesh != _rootMesh) {
          _groundMirror->renderList().emplace_back(mesh.get());
        }
      }
    }
  }

  _groundMirror->clearColor         = Color4(_options.groundColor.r, //
                                     _options.groundColor.g, //
                                     _options.groundColor.b, //
                                     1.f);
  _groundMirror->adaptiveBlurKernel = _options.groundMirrorBlurKernel;
}

void EnvironmentHelper::_setupMirrorInGroundMaterial()
{
  if (_groundMaterial) {
    _groundMaterial->reflectionTexture = _groundMirror;
    _groundMaterial->reflectionFresnel = true;
    _groundMaterial->reflectionAmount  = _options.groundMirrorAmount;
    _groundMaterial->reflectionStandardFresnelWeight
      = _options.groundMirrorFresnelWeight;
    _groundMaterial->reflectionFalloffDistance
      = _options.groundMirrorFallOffDistance;
  }
}

void EnvironmentHelper::_setupSkybox(const ISceneSize& sceneSize)
{
  if (!_skybox || _skybox->isDisposed()) {
    _skybox = Mesh::CreateBox("BackgroundSkybox", sceneSize.skyboxSize, _scene,
                              false, Mesh::BACKSIDE);
    _skybox->onDisposeObservable.add(
      [this](Node* /*node*/, EventState& /*es*/) { _skybox = nullptr; });
  }
  _skybox->parent = _rootMesh.get();
}

void EnvironmentHelper::_setupSkyboxMaterial()
{
  if (!_skybox) {
    return;
  }

  if (!_skyboxMaterial) {
    _skyboxMaterial
      = BackgroundMaterial::New("BackgroundSkyboxMaterial", _scene);
  }
  _skyboxMaterial->useRGBColor  = false;
  _skyboxMaterial->primaryColor = _options.skyboxColor;
  _skyboxMaterial->enableNoise  = true;

  _skybox->material = _skyboxMaterial;
}

void EnvironmentHelper::_setupSkyboxReflectionTexture()
{
  if (!_skyboxMaterial) {
    return;
  }

  if (_skyboxTexture) {
    return;
  }

  if (std::holds_alternative<BaseTexturePtr>(_options.skyboxTexture)) {
    _skyboxMaterial->reflectionTexture
      = std::get<BaseTexturePtr>(_options.skyboxTexture);
    return;
  }

  _skyboxTexture
    = CubeTexture::New(std::get<std::string>(_options.skyboxTexture), _scene);
  _skyboxTexture->coordinatesMode    = TextureConstants::SKYBOX_MODE;
  _skyboxTexture->gammaSpace         = false;
  _skyboxMaterial->reflectionTexture = _skyboxTexture;
}

void EnvironmentHelper::_errorHandler(const std::string& message,
                                      const std::exception& exception)
{
  Exception _exception{message, exception};
  onErrorObservable.notifyObservers(&_exception);
}

void EnvironmentHelper::dispose()
{
  if (_groundMaterial) {
    _groundMaterial->dispose(true, true);
  }
  if (_skyboxMaterial) {
    _skyboxMaterial->dispose(true, true);
  }
  _rootMesh->dispose(false);
}

} // end of namespace BABYLON
