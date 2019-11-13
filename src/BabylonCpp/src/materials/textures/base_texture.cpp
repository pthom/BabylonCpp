#include <babylon/materials/textures/base_texture.h>

#include <nlohmann/json.hpp>

#include <babylon/core/array_buffer_view.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/materials/material.h>
#include <babylon/materials/textures/internal_texture.h>
#include <babylon/materials/textures/texture.h>
#include <babylon/math/spherical_polynomial.h>
#include <babylon/misc/guid.h>
#include <babylon/misc/highdynamicrange/cube_map_to_spherical_polynomial_tools.h>

namespace BABYLON {

BaseTexture::BaseTexture(Scene* scene)
    : hasAlpha{this, &BaseTexture::get_hasAlpha, &BaseTexture::set_hasAlpha}
    , getAlphaFromRGB{false}
    , level{1.f}
    , coordinatesIndex{0}
    , coordinatesMode{this, &BaseTexture::get_coordinatesMode,
                      &BaseTexture::set_coordinatesMode}
    , wrapU{Constants::TEXTURE_WRAP_ADDRESSMODE}
    , wrapV{Constants::TEXTURE_WRAP_ADDRESSMODE}
    , wrapR{Constants::TEXTURE_WRAP_ADDRESSMODE}
    , anisotropicFilteringLevel{BaseTexture::
                                  DEFAULT_ANISOTROPIC_FILTERING_LEVEL}
    , isCube{this, &BaseTexture::get_isCube, &BaseTexture::set_isCube}
    , is3D{this, &BaseTexture::get_is3D, &BaseTexture::set_is3D}
    , gammaSpace{true}
    , isRGBD{this, &BaseTexture::get_isRGBD}
    , invertZ{false}
    , noMipmap{this, &BaseTexture::get_noMipmap}
    , lodLevelInAlpha{false}
    , lodGenerationOffset{this, &BaseTexture::get_lodGenerationOffset,
                          &BaseTexture::set_lodGenerationOffset}
    , lodGenerationScale{this, &BaseTexture::get_lodGenerationScale,
                         &BaseTexture::set_lodGenerationScale}
    , linearSpecularLOD{this, &BaseTexture::get_linearSpecularLOD,
                        &BaseTexture::set_linearSpecularLOD}
    , irradianceTexture{this, &BaseTexture::get_irradianceTexture,
                        &BaseTexture::set_irradianceTexture}
    , isRenderTarget{false}
    , uid{this, &BaseTexture::get_uid}
    , onDispose{this, &BaseTexture::set_onDispose}
    , delayLoadState{Constants::DELAYLOADSTATE_NONE}
    , _texture{nullptr}
    , isBlocking{this, &BaseTexture::get_isBlocking,
                 &BaseTexture::set_isBlocking}
    , boundingBoxSize{this, &BaseTexture::get_boundingBoxSize,
                      &BaseTexture::set_boundingBoxSize}
    , textureType{this, &BaseTexture::get_textureType}
    , textureFormat{this, &BaseTexture::get_textureFormat}
    , sphericalPolynomial{this, &BaseTexture::get_sphericalPolynomial,
                          &BaseTexture::set_sphericalPolynomial}
    , _lodTextureHigh{this, &BaseTexture::get__lodTextureHigh}
    , _lodTextureMid{this, &BaseTexture::get__lodTextureMid}
    , _lodTextureLow{this, &BaseTexture::get__lodTextureLow}
    , _hasAlpha{false}
    , _coordinatesMode{Constants::TEXTURE_EXPLICIT_MODE}
    , _scene{scene ? scene : Engine::LastCreatedScene()}
    , _uid{GUID::RandomId()}
    , _onDisposeObserver{nullptr}
    , _textureMatrix{Matrix::IdentityReadOnly()}
    , _reflectionTextureMatrix{Matrix::IdentityReadOnly()}
    , emptyVector3{std::nullopt}
    , _cachedSize{Size::Zero()}
    , _nullSphericalPolynomial{nullptr}
    , _nullBaseTexture{nullptr}
{
}

BaseTexture::~BaseTexture() = default;

Type BaseTexture::type() const
{
  return Type::BASETEXTURE;
}

void BaseTexture::addToScene(const BaseTexturePtr& newTexture)
{
  if (_scene) {
    uniqueId = _scene->getUniqueId();
    _scene->textures.emplace_back(newTexture);
  }
}

void BaseTexture::set_hasAlpha(bool value)
{
  if (_hasAlpha == value) {
    return;
  }
  _hasAlpha = value;
  if (_scene) {
    _scene->markAllMaterialsAsDirty(Constants::MATERIAL_TextureDirtyFlag
                                    | Constants::MATERIAL_MiscDirtyFlag);
  }
}

bool BaseTexture::get_hasAlpha() const
{
  return _hasAlpha;
}

void BaseTexture::set_coordinatesMode(unsigned int value)
{
  if (_coordinatesMode == value) {
    return;
  }
  _coordinatesMode = value;
  if (_scene) {
    _scene->markAllMaterialsAsDirty(Constants::MATERIAL_TextureDirtyFlag);
  }
}

unsigned int BaseTexture::get_coordinatesMode() const
{
  return _coordinatesMode;
}

bool BaseTexture::get_isCube() const
{
  if (!_texture) {
    return false;
  }

  return _texture->isCube;
}

void BaseTexture::set_isCube(bool value)
{
  if (!_texture) {
    return;
  }

  _texture->isCube = value;
}

bool BaseTexture::get_is3D() const
{
  if (!_texture) {
    return false;
  }

  return _texture->is3D;
}

void BaseTexture::set_is3D(bool value)
{
  if (!_texture) {
    return;
  }

  _texture->is3D = value;
}

bool BaseTexture::get_noMipmap() const
{
  return false;
}

bool BaseTexture::get_isRGBD() const
{
  return _texture != nullptr && _texture->_isRGBD;
}

float BaseTexture::get_lodGenerationOffset() const
{
  if (_texture) {
    return _texture->_lodGenerationOffset;
  }

  return 0.f;
}

void BaseTexture::set_lodGenerationOffset(float value)
{
  if (_texture) {
    _texture->_lodGenerationOffset = value;
  }
}

float BaseTexture::get_lodGenerationScale() const
{
  if (_texture) {
    return _texture->_lodGenerationScale;
  }

  return 0.f;
}

void BaseTexture::set_lodGenerationScale(float value)
{
  if (_texture) {
    _texture->_lodGenerationScale = value;
  }
}

bool BaseTexture::get_linearSpecularLOD() const
{
  if (_texture) {
    return _texture->_linearSpecularLOD;
  }

  return false;
}

void BaseTexture::set_linearSpecularLOD(bool value)
{
  if (_texture) {
    _texture->_linearSpecularLOD = value;
  }
}

BaseTexturePtr& BaseTexture::get_irradianceTexture()
{
  if (_texture) {
    return _texture->_irradianceTexture;
  }

  return _nullBaseTexture;
}

void BaseTexture::set_irradianceTexture(const BaseTexturePtr& value)
{
  if (_texture) {
    _texture->_irradianceTexture = value;
  }
}

std::string BaseTexture::get_uid() const
{
  return _uid;
}

std::string BaseTexture::toString() const
{
  return name;
}

const std::string BaseTexture::getClassName() const
{
  return "BaseTexture";
}

void BaseTexture::set_onDispose(
  const std::function<void(BaseTexture*, EventState&)>& callback)
{
  if (_onDisposeObserver) {
    onDisposeObservable.remove(_onDisposeObserver);
  }
  _onDisposeObserver = onDisposeObservable.add(callback);
}

bool BaseTexture::get_isBlocking() const
{
  return true;
}

void BaseTexture::set_isBlocking(bool /*value*/)
{
}

Scene* BaseTexture::getScene() const
{
  return _scene;
}

Matrix* BaseTexture::getTextureMatrix()
{
  return &_textureMatrix;
}

Matrix* BaseTexture::getReflectionTextureMatrix()
{
  return &_reflectionTextureMatrix;
}

InternalTexturePtr& BaseTexture::getInternalTexture()
{
  return _texture;
}

bool BaseTexture::isReadyOrNotBlocking()
{
  return !isBlocking() || isReady();
}

bool BaseTexture::isReady()
{
  if (delayLoadState == Constants::DELAYLOADSTATE_NOTLOADED) {
    delayLoad();
    return false;
  }

  if (_texture) {
    return _texture->isReady;
  }

  return false;
}

ISize BaseTexture::getSize()
{
  if (_texture) {
    if (_texture->width) {
      _cachedSize.width  = _texture->width;
      _cachedSize.height = _texture->height;
      return _cachedSize;
    }

    if (_texture->_size) {
      _cachedSize.width  = _texture->_size;
      _cachedSize.height = _texture->_size;
      return _cachedSize;
    }
  }

  return _cachedSize;
}

ISize BaseTexture::getBaseSize()
{
  if (!isReady() || !_texture) {
    return Size::Zero();
  }

  if (_texture->_size) {
    return Size(_texture->_size, _texture->_size);
  }

  return Size(_texture->baseWidth, _texture->baseHeight);
}

void BaseTexture::updateSamplingMode(unsigned int samplingMode)
{
  if (!_texture) {
    return;
  }

  auto scene = getScene();

  if (!scene) {
    return;
  }

  scene->getEngine()->updateTextureSamplingMode(samplingMode, _texture);
}

void BaseTexture::scale(float /*ratio*/)
{
}

bool BaseTexture::canRescale()
{
  return false;
}

InternalTexturePtr
BaseTexture::_getFromCache(const std::string& url, bool iNoMipmap,
                           unsigned int sampling,
                           const std::optional<bool>& invertY)
{
  if (!_scene) {
    return nullptr;
  }

  auto& texturesCache = _scene->getEngine()->getLoadedTexturesCache();
  for (auto& texturesCacheEntry : texturesCache) {
    if (!invertY.has_value() || *invertY == texturesCacheEntry->invertY) {
      if ((texturesCacheEntry->url.compare(url) == 0)
          && texturesCacheEntry->generateMipMaps != iNoMipmap) {
        if (!sampling || sampling == texturesCacheEntry->samplingMode) {
          texturesCacheEntry->incrementReferences();
          return texturesCacheEntry;
        }
      }
    }
  }

  return nullptr;
}

void BaseTexture::_rebuild()
{
}

void BaseTexture::delayLoad(const std::string& /*forcedExtension*/)
{
}

void BaseTexture::set_boundingBoxSize(const std::optional<Vector3>& /*value*/)
{
}

std::optional<Vector3>& BaseTexture::get_boundingBoxSize()
{
  return emptyVector3;
}

std::vector<AnimationPtr> BaseTexture::getAnimations()
{
  return animations;
}

std::unique_ptr<BaseTexture> BaseTexture::clone() const
{
  return nullptr;
}

unsigned int BaseTexture::get_textureType() const
{
  if (!_texture) {
    return Constants::TEXTURETYPE_UNSIGNED_INT;
  }

  return _texture->type ? _texture->type : Constants::TEXTURETYPE_UNSIGNED_INT;
}

unsigned int BaseTexture::get_textureFormat() const
{
  if (!_texture) {
    return Constants::TEXTUREFORMAT_RGBA;
  }

  return _texture->format ? _texture->format : Constants::TEXTUREFORMAT_RGBA;
}

ArrayBufferView BaseTexture::readPixels(unsigned int faceIndex, int iLevel,
                                        std::optional<ArrayBufferView> buffer)
{
  if (!_texture) {
    return ArrayBufferView();
  }

  auto size   = getSize();
  auto width  = size.width;
  auto height = size.height;
  auto scene  = getScene();

  if (!scene) {
    return ArrayBufferView();
  }

  auto engine = scene->getEngine();

  if (iLevel != 0) {
    width  = width / static_cast<int>(std::pow(2, iLevel));
    height = height / static_cast<int>(std::pow(2, iLevel));

    width  = static_cast<int>(std::round(width));
    height = static_cast<int>(std::round(height));
  }

  if (_texture->isCube) {
    return engine->_readTexturePixels(_texture, size.width, size.height,
                                      static_cast<int>(faceIndex), iLevel,
                                      buffer);
  }

  return engine->_readTexturePixels(_texture, size.width, size.height, -1,
                                    iLevel, buffer);
}

void BaseTexture::releaseInternalTexture()
{
  if (_texture) {
    _texture->dispose();
    _texture = nullptr;
  }
}

SphericalPolynomialPtr& BaseTexture::get_sphericalPolynomial()
{
  if (!_texture || !isReady()) {
    return _nullSphericalPolynomial;
  }

  if (!_texture->_sphericalPolynomial) {
    _texture->_sphericalPolynomial = CubeMapToSphericalPolynomialTools::
      ConvertCubeMapTextureToSphericalPolynomial(*this);
  }

  return _texture->_sphericalPolynomial;
}

void BaseTexture::set_sphericalPolynomial(const SphericalPolynomialPtr& value)
{
  if (_texture) {
    _texture->_sphericalPolynomial = value;
  }
}

BaseTexturePtr& BaseTexture::get__lodTextureHigh()
{
  if (_texture) {
    return _texture->_lodTextureHigh;
  }
  return _nullBaseTexture;
}

BaseTexturePtr& BaseTexture::get__lodTextureMid()
{
  if (_texture) {
    return _texture->_lodTextureMid;
  }
  return _nullBaseTexture;
}

BaseTexturePtr& BaseTexture::get__lodTextureLow()
{
  if (_texture) {
    return _texture->_lodTextureLow;
  }
  return _nullBaseTexture;
}

void BaseTexture::dispose()
{
  if (!_scene) {
    return;
  }

  // Animations
  _scene->stopAnimation(this);

  // Remove from scene
  _scene->textures.erase(
    std::remove_if(_scene->textures.begin(), _scene->textures.end(),
                   [this](const BaseTexturePtr& baseTexture) {
                     return baseTexture.get() == this;
                   }),
    _scene->textures.end());

  _scene->onTextureRemovedObservable.notifyObservers(this);

  if (_texture == nullptr) {
    return;
  }

  // Release
  releaseInternalTexture();

  // Callback
  onDisposeObservable.notifyObservers(this);
  onDisposeObservable.clear();
}

json BaseTexture::serialize() const
{
  return nullptr;
}

void BaseTexture::WhenAllReady(const std::vector<BaseTexture*>& textures,
                               const std::function<void()>& callback)
{
  auto numRemaining = textures.size();
  if (numRemaining == 0) {
    callback();
    return;
  }

  for (auto& texture : textures) {
    if (texture->isReady()) {
      if (--numRemaining == 0) {
        callback();
      }
    }
    else {
      auto onLoadObservable
        = static_cast<Texture*>(texture)->onLoadObservable();

      const std::function<void(Texture*, EventState&)> onLoadCallback
        = [&](Texture*, EventState&) {
            onLoadObservable.removeCallback(onLoadCallback);
            if (--numRemaining == 0) {
              callback();
            }
          };

      onLoadObservable.add(onLoadCallback);
    }
  }
}

} // end of namespace BABYLON
