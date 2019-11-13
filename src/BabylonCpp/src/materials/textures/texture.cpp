#include <babylon/materials/textures/texture.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/core/json_util.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/materials/material.h>
#include <babylon/materials/textures/cube_texture.h>
#include <babylon/materials/textures/internal_texture.h>
#include <babylon/materials/textures/mirror_texture.h>
#include <babylon/meshes/buffer.h>
#include <babylon/misc/serialization_helper.h>
#include <babylon/misc/tools.h>

namespace BABYLON {

bool Texture::UseSerializedUrlIfAny = false;

Texture::Texture(
  const std::string& iUrl, Scene* scene, bool iNoMipmap, bool invertY,
  unsigned int samplingMode, const std::function<void()>& onLoad,
  const std::function<void(const std::string& message,
                           const std::string& exception)>& onError,
  const std::optional<std::variant<std::string, ArrayBuffer, Image>>& buffer,
  bool deleteBuffer, const std::optional<unsigned int>& format)
    : BaseTexture{scene}
    , uOffset{0.f}
    , vOffset{0.f}
    , uScale{1.0}
    , vScale{1.0}
    , uAng{0.f}
    , vAng{0.f}
    , wAng{0.f}
    , uRotationCenter{0.5f}
    , vRotationCenter{0.5f}
    , wRotationCenter{0.5f}
    , samplingMode{this, &Texture::get_samplingMode}
    , invertY{this, &Texture::get_invertY}
    , onLoadObservable{this, &Texture::get_onLoadObservable}
    , _isBlocking{true}
    , _rowGenerationMatrix{nullptr}
    , _cachedTextureMatrix{nullptr}
    , _projectionModeMatrix{nullptr}
    , _t0{nullptr}
    , _t1{nullptr}
    , _t2{nullptr}
    , _cachedUOffset{-1.f}
    , _cachedVOffset{-1.f}
    , _cachedUScale{0.f}
    , _cachedVScale{0.f}
    , _cachedUAng{-1.f}
    , _cachedVAng{-1.f}
    , _cachedWAng{-1.f}
    , _cachedProjectionMatrixId{-1}
    , _cachedCoordinatesMode{-1}
    , _buffer{buffer}
    , _delayedOnLoad{nullptr}
    , _delayedOnError{nullptr}
    , _onLoad{onLoad}
{
  name                 = iUrl;
  url                  = iUrl;
  _noMipmap            = iNoMipmap;
  _invertY             = invertY;
  _initialSamplingMode = samplingMode;
  _buffer              = buffer;
  _deleteBuffer        = deleteBuffer;
  if (format) {
    _format = format;
  }

  scene = getScene();

  if (!scene) {
    return;
  }
  scene->getEngine()->onBeforeTextureInitObservable.notifyObservers(this);

  _load = [this](InternalTexture*, EventState&) {
    if (_texture && _texture->_invertVScale) {
      vScale = -1.f;
    }
    if (_onLoadObservable.hasObservers()) {
      _onLoadObservable.notifyObservers(this);
    }
    if (_onLoad) {
      _onLoad();
    }

    auto scene = getScene();
    if (!isBlocking() && scene) {
      scene->resetCachedMaterial();
    }
  };

  if (url.empty()) {
    _delayedOnLoad  = _load;
    _delayedOnError = onError;
    return;
  }

  _texture = _getFromCache(url, noMipmap, samplingMode, invertY);

  if (!_texture) {
    if (!scene->useDelayedTextureLoading) {
      _texture = scene->getEngine()->createTexture(
        url, noMipmap, invertY, scene, samplingMode, _load, onError, _buffer,
        nullptr, _format);
      if (deleteBuffer) {
        _buffer = std::nullopt;
      }
    }
    else {
      delayLoadState = Constants::DELAYLOADSTATE_NOTLOADED;

      _delayedOnLoad  = _load;
      _delayedOnError = onError;
    }
  }
  else {
    if (_texture->isReady) {
      Tools::SetImmediate([this]() {
        EventState es{-1};
        _load(nullptr, es);
      });
    }
    else {
      _texture->onLoadedObservable.add(_load);
    }
  }
}

Texture::~Texture() = default;

Type Texture::type() const
{
  return Type::TEXTURE;
}

const char* Texture::getClassName() const
{
  return "Texture";
}

void Texture::set_isBlocking(bool value)
{
  _isBlocking = value;
}

bool Texture::get_isBlocking() const
{
  return _isBlocking;
}

unsigned int Texture::get_samplingMode() const
{
  if (!_texture) {
    return _initialSamplingMode;
  }

  return _texture->samplingMode;
}

bool Texture::get_invertY() const
{
  return _invertY;
}

bool Texture::get_noMipmap() const
{
  return _noMipmap;
}

void Texture::updateURL(
  const std::string& iUrl,
  const std::optional<std::variant<std::string, ArrayBuffer, Image>>& buffer,
  const std::function<void(InternalTexture*, EventState&)>& onLoad)
{
  if (iUrl.empty()) {
    releaseInternalTexture();
    getScene()->markAllMaterialsAsDirty(Constants::MATERIAL_TextureDirtyFlag);
  }

  url            = iUrl;
  _buffer        = buffer;
  delayLoadState = Constants::DELAYLOADSTATE_NOTLOADED;

  if (onLoad) {
    _delayedOnLoad = onLoad;
  }
  delayLoad();
}

void Texture::delayLoad(const std::string& /*forcedExtension*/)
{
  if (delayLoadState != Constants::DELAYLOADSTATE_NOTLOADED) {
    return;
  }

  auto scene = getScene();

  if (!scene) {
    return;
  }

  delayLoadState = Constants::DELAYLOADSTATE_LOADED;
  _texture       = _getFromCache(url, _noMipmap, samplingMode, _invertY);

  if (!_texture) {
    _texture = scene->getEngine()->createTexture(
      url, _noMipmap, _invertY, getScene(), samplingMode, _delayedOnLoad,
      _delayedOnError, _buffer, nullptr, _format);
    if (_deleteBuffer) {
      // delete _buffer;
    }
  }
  else {
    if (_delayedOnLoad) {
      if (_texture->isReady) {
        Tools::SetImmediate([this]() {
          EventState es{-1};
          _delayedOnLoad(nullptr, es);
        });
      }
      else {
        _texture->onLoadedObservable.add(_delayedOnLoad);
      }
    }
  }

  _delayedOnLoad  = nullptr;
  _delayedOnError = nullptr;
}

void Texture::_prepareRowForTextureGeneration(float x, float y, float z,
                                              Vector3& t)
{
  x *= uScale;
  y *= vScale;

  x -= uRotationCenter * uScale;
  y -= vRotationCenter * vScale;
  z -= wRotationCenter;

  Vector3::TransformCoordinatesFromFloatsToRef(x, y, z, *_rowGenerationMatrix,
                                               t);

  t.x += uRotationCenter * uScale + uOffset;
  t.y += vRotationCenter * vScale + vOffset;
  t.z += wRotationCenter;
}

Matrix* Texture::getTextureMatrix()
{
  if (stl_util::almost_equal(uOffset, _cachedUOffset)
      && stl_util::almost_equal(vOffset, _cachedVOffset)
      && stl_util::almost_equal(uScale, _cachedUScale)
      && stl_util::almost_equal(vScale, _cachedVScale)
      && stl_util::almost_equal(uAng, _cachedUAng)
      && stl_util::almost_equal(vAng, _cachedVAng)
      && stl_util::almost_equal(wAng, _cachedWAng)) {
    return _cachedTextureMatrix.get();
  }

  _cachedUOffset = uOffset;
  _cachedVOffset = vOffset;
  _cachedUScale  = uScale;
  _cachedVScale  = vScale;
  _cachedUAng    = uAng;
  _cachedVAng    = vAng;
  _cachedWAng    = wAng;

  if (!_cachedTextureMatrix) {
    _cachedTextureMatrix = std::make_unique<Matrix>(Matrix::Zero());
    _rowGenerationMatrix = std::make_unique<Matrix>();
    _t0                  = std::make_unique<Vector3>(Vector3::Zero());
    _t1                  = std::make_unique<Vector3>(Vector3::Zero());
    _t2                  = std::make_unique<Vector3>(Vector3::Zero());
  }

  Matrix::RotationYawPitchRollToRef(vAng, uAng, wAng, *_rowGenerationMatrix);

  _prepareRowForTextureGeneration(0.f, 0.f, 0.f, *_t0);
  _prepareRowForTextureGeneration(1.f, 0.f, 0.f, *_t1);
  _prepareRowForTextureGeneration(0.f, 1.f, 0.f, *_t2);

  _t1->subtractInPlace(*_t0);
  _t2->subtractInPlace(*_t0);

  Matrix::FromValuesToRef(_t1->x, _t1->y, _t1->z, 0.f, //
                          _t2->x, _t2->y, _t2->z, 0.f, //
                          _t0->x, _t0->y, _t0->z, 0.f, //
                          0.f, 0.f, 0.f, 1.f,          //
                          *_cachedTextureMatrix);

  auto scene = getScene();

  if (!scene) {
    return _cachedTextureMatrix.get();
  }

  scene->markAllMaterialsAsDirty(
    Constants::MATERIAL_TextureDirtyFlag, [this](Material* mat) {
      return mat->hasTexture(shared_from_base<Texture>());
    });

  return _cachedTextureMatrix.get();
}

Matrix* Texture::getReflectionTextureMatrix()
{
  auto scene = getScene();
  if (!scene) {
    return _cachedTextureMatrix.get();
  }

  if (stl_util::almost_equal(uOffset, _cachedUOffset)
      && stl_util::almost_equal(vOffset, _cachedVOffset)
      && stl_util::almost_equal(uScale, _cachedUScale)
      && stl_util::almost_equal(vScale, _cachedVScale)
      && (static_cast<int>(coordinatesMode()) == _cachedCoordinatesMode)) {
    if (coordinatesMode() == TextureConstants::PROJECTION_MODE) {
      if (_cachedProjectionMatrixId
          == scene->getProjectionMatrix().updateFlag) {
        return _cachedTextureMatrix.get();
      }
    }
    else {
      return _cachedTextureMatrix.get();
    }
  }

  if (!_cachedTextureMatrix) {
    _cachedTextureMatrix = std::make_unique<Matrix>(Matrix::Zero());
  }

  if (!_projectionModeMatrix) {
    _projectionModeMatrix = std::make_unique<Matrix>(Matrix::Zero());
  }

  _cachedUOffset         = uOffset;
  _cachedVOffset         = vOffset;
  _cachedUScale          = uScale;
  _cachedVScale          = vScale;
  _cachedCoordinatesMode = static_cast<int>(coordinatesMode());

  switch (coordinatesMode()) {
    case TextureConstants::PLANAR_MODE: {
      Matrix::IdentityToRef(*_cachedTextureMatrix);
      _cachedTextureMatrix->setAtIndex(0, uScale);
      _cachedTextureMatrix->setAtIndex(5, vScale);
      _cachedTextureMatrix->setAtIndex(12, uOffset);
      _cachedTextureMatrix->setAtIndex(13, vOffset);
    } break;
    case TextureConstants::PROJECTION_MODE: {
      Matrix::FromValuesToRef(0.5f, 0.f, 0.f, 0.f,  //
                              0.f, -0.5f, 0.f, 0.f, //
                              0.f, 0.f, 0.f, 0.f,   //
                              0.5f, 0.5f, 1.f, 1.f, //
                              *_projectionModeMatrix);

      auto projectionMatrix     = scene->getProjectionMatrix();
      _cachedProjectionMatrixId = projectionMatrix.updateFlag;
      projectionMatrix.multiplyToRef(*_projectionModeMatrix,
                                     *_cachedTextureMatrix);
    } break;
    default:
      Matrix::IdentityToRef(*_cachedTextureMatrix);
      break;
  }

  scene->markAllMaterialsAsDirty(
    Constants::MATERIAL_TextureDirtyFlag, [this](Material* mat) {
      // we need to copy mat->getActiveTextures, since it returns by value!
      auto activeTextures = mat->getActiveTextures();
      return (std::find_if(activeTextures.begin(),
                           activeTextures.end(),
                           [this](const BaseTexturePtr& texture) {
                             return texture.get() == this;
                           })
              != activeTextures.end());
    });

  return _cachedTextureMatrix.get();
}

TexturePtr Texture::clone() const
{
  std::optional<std::variant<std::string, ArrayBuffer, Image>> nullBuffer
    = std::nullopt;
  auto newTexture = Texture::New(
    _texture ? _texture->url : "", getScene(), _noMipmap, _invertY,
    samplingMode, nullptr, nullptr, _texture ? _texture->_buffer : nullBuffer);

  // Base texture
  newTexture->hasAlpha         = hasAlpha();
  newTexture->level            = level;
  newTexture->wrapU            = wrapU;
  newTexture->wrapV            = wrapV;
  newTexture->coordinatesIndex = coordinatesIndex;
  newTexture->coordinatesMode  = coordinatesMode();

  // Texture
  newTexture->uOffset = uOffset;
  newTexture->vOffset = vOffset;
  newTexture->uScale  = uScale;
  newTexture->vScale  = vScale;
  newTexture->uAng    = uAng;
  newTexture->vAng    = vAng;
  newTexture->wAng    = wAng;

  return newTexture;
}

Observable<Texture>& Texture::get_onLoadObservable()
{
  return _onLoadObservable;
}

json Texture::serialize() const
{
  return nullptr;
}

void Texture::dispose()
{
  BaseTexture::dispose();

  _onLoadObservable.clear();

  _delayedOnLoad  = nullptr;
  _delayedOnError = nullptr;
}

TexturePtr Texture::CreateFromBase64String(
  const std::string& data, const std::string& iName, Scene* scene,
  bool iNoMipmap, bool invertY, unsigned int samplingMode,
  const std::function<void()>& onLoad,
  const std::function<void(const std::string& message,
                           const std::string& exception)>& onError,
  unsigned int format)
{
  return Texture::New("data:" + iName, scene, iNoMipmap, invertY, samplingMode,
                      onLoad, onError, data, false, format);
}

BaseTexturePtr Texture::Parse(const json& parsedTexture, Scene* scene,
                              const std::string& rootUrl)
{
  if (json_util::has_valid_key_value(parsedTexture, "isCube")
      && json_util::get_bool(parsedTexture, "isCube")) {
    return CubeTexture::Parse(parsedTexture, scene, rootUrl);
  }

  if ((!json_util::has_valid_key_value(parsedTexture, "name")
       && (!json_util::has_valid_key_value(parsedTexture, "isRenderTarget")))) {
    return nullptr;
  }

  auto texture = SerializationHelper::Parse(
    [&]() -> TexturePtr {
      auto generateMipMaps = true;
      if (json_util::has_valid_key_value(parsedTexture, "noMipmap")
          && json_util::get_bool(parsedTexture, "noMipmap")) {
        generateMipMaps = true;
      }
      if (json_util::has_valid_key_value(parsedTexture, "mirrorPlane")) {
        auto mirrorTexture = MirrorTexture::New(
          json_util::get_string(parsedTexture, "name"),
          json_util::get_number<float>(parsedTexture, "renderTargetSize"),
          scene, generateMipMaps);
        mirrorTexture->_waitingRenderList
          = json_util::get_array<std::string>(parsedTexture, "renderList");
        mirrorTexture->mirrorPlane = Plane::FromArray(
          json_util::get_array<float>(parsedTexture, "mirrorPlane"));

        return mirrorTexture;
      }
      else if (json_util::has_valid_key_value(parsedTexture, "isRenderTarget")
               && json_util::get_bool(parsedTexture, "isRenderTarget")) {
        auto renderTargetTexture = RenderTargetTexture::New(
          json_util::get_string(parsedTexture, "name"),
          json_util::get_number<float>(parsedTexture, "renderTargetSize"),
          scene, generateMipMaps);
        renderTargetTexture->_waitingRenderList
          = json_util::get_array<std::string>(parsedTexture, "renderList");

        return renderTargetTexture;
      }
      else {
        TexturePtr texture = nullptr;
        if (json_util::has_valid_key_value(parsedTexture, "base64String")) {
          texture = Texture::CreateFromBase64String(
            json_util::get_string(parsedTexture, "base64String"),
            json_util::get_string(parsedTexture, "name"), scene,
            !generateMipMaps);
        }
        else {
          auto url2 = rootUrl + json_util::get_string(parsedTexture, "name");

          if (Texture::UseSerializedUrlIfAny
              && json_util::has_valid_key_value(parsedTexture, "url")) {
            url2 = json_util::get_string(parsedTexture, "url");
          }
          texture
            = Texture::New(url2, scene, !generateMipMaps,
                           json_util::get_bool(parsedTexture, "invertY", true));
        }

        return texture;
      }
    },
    parsedTexture, scene);

  // Update Sampling Mode
  if (json_util::has_valid_key_value(parsedTexture, "samplingMode")) {
    auto sampling
      = json_util::get_number<unsigned>(parsedTexture, "samplingMode");
    if (texture->samplingMode != sampling) {
      texture->updateSamplingMode(sampling);
    }
  }

  // Animations
  if (json_util::has_key(parsedTexture, "animations")) {
    for (auto parsedAnimation :
         json_util::get_array<json>(parsedTexture, "animations")) {
      texture->animations.emplace_back(Animation::Parse(parsedAnimation));
    }
  }

  return texture;
}

TexturePtr Texture::LoadFromDataString(
  const std::string& iName,
  const std::optional<std::variant<std::string, ArrayBuffer, Image>>& buffer,
  Scene* scene, bool deleteBuffer, bool iNoMipmap, bool invertY,
  unsigned int samplingMode, const std::function<void()>& onLoad,
  const std::function<void(const std::string& message,
                           const std::string& exception)>& onError,
  unsigned int format)
{
  std::string _name = iName;
  if (_name.substr(0, 5) != "data:") {
    _name = "data:" + iName;
  }

  return Texture::New(iName, scene, iNoMipmap, invertY, samplingMode, onLoad,
                      onError, buffer, deleteBuffer, format);
}

MirrorTexturePtr
Texture::_CreateMirror(const std::string& iName,
                       const std::variant<ISize, float>& renderTargetSize,
                       Scene* scene, bool generateMipMaps)
{
  return MirrorTexture::New(iName, renderTargetSize, scene, generateMipMaps);
}

} // end of namespace BABYLON
