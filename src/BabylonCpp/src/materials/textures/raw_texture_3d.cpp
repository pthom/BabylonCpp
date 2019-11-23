﻿#include <babylon/materials/textures/raw_texture_3d.h>

#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/materials/textures/internal_texture.h>

namespace BABYLON {

RawTexture3D::RawTexture3D(const ArrayBufferView& data, int width, int height,
                           int depth, unsigned int iFormat, Scene* scene,
                           bool generateMipMaps, bool iInvertY,
                           unsigned int iSamplingMode,
                           unsigned int iTextureType)
    : Texture{nullptr, scene, !generateMipMaps, iInvertY}, format{iFormat}
{
  _engine = scene->getEngine();

  _texture = scene->getEngine()->createRawTexture3D(data.uint8Span(),
                                                    width,
                                                    height,
                                                    depth,
                                                    iFormat,
                                                    generateMipMaps,
                                                    invertY,
                                                    iSamplingMode,
                                                    "",
                                                    iTextureType
  );

  is3D = true;
}

RawTexture3D::~RawTexture3D() = default;

void RawTexture3D::update(const ArrayBufferView& data)
{
  if (!_texture) {
    return;
  }
  _engine->updateRawTexture3D(_texture, data.uint8Span(), _texture->format,
                              _texture->invertY, "", _texture->type);
}

} // end of namespace BABYLON
