#ifndef BABYLON_MATERIALS_TEXTURES_RAW_TEXTURE_3D_H
#define BABYLON_MATERIALS_TEXTURES_RAW_TEXTURE_3D_H

#include <babylon/babylon_api.h>
#include <babylon/engines/constants.h>
#include <babylon/materials/textures/texture.h>

namespace BABYLON {

class Engine;

/**
 * @brief Class used to store 3D textures containing user data.
 */
class BABYLON_SHARED_EXPORT RawTexture3D : public Texture {

public:
  /**
   * @brief Create a new RawTexture3D.
   * @param data defines the data of the texture
   * @param width defines the width of the texture
   * @param height defines the height of the texture
   * @param depth defines the depth of the texture
   * @param format defines the texture format to use
   * @param scene defines the hosting scene
   * @param generateMipMaps defines a boolean indicating if mip levels should be
   * generated (true by default)
   * @param invertY defines if texture must be stored with Y axis inverted
   * @param samplingMode defines the sampling mode to use
   * (Texture.TRILINEAR_SAMPLINGMODE by default)
   * @param textureType defines the texture Type
   * (Engine.TEXTURETYPE_UNSIGNED_INT, Engine.TEXTURETYPE_FLOAT...)
   */
  RawTexture3D(const ArrayBufferView& data, int width, int height, int depth,
               unsigned int format, Scene* scene, bool generateMipMaps = true,
               bool invertY = false,
               unsigned int samplingMode
               = TextureConstants::TRILINEAR_SAMPLINGMODE,
               unsigned int textureType = Constants::TEXTURETYPE_UNSIGNED_INT);
  ~RawTexture3D(); // = default

  /**
   * @brief Update the texture with new data.
   * @param data defines the data to store in the texture
   */
  void update(const ArrayBufferView& data);

public:
  /**
   * Gets or sets the texture format to use
   */
  unsigned int format;

private:
  Engine* _engine;

}; // end of struct of class RawTexture3D

} // end of namespace BABYLON

#endif // end of BABYLON_MATERIALS_TEXTURES_RAW_TEXTURE_3D_H
