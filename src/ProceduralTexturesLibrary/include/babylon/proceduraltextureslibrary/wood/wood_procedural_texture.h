#ifndef BABYLON_PROCEDURAL_TEXTURES_LIBRARY_WOOD_WOOD_PROCEDURAL_TEXTURE_H
#define BABYLON_PROCEDURAL_TEXTURES_LIBRARY_WOOD_WOOD_PROCEDURAL_TEXTURE_H

#include <babylon/babylon_api.h>
#include <babylon/materials/textures/procedurals/procedural_texture.h>
#include <babylon/math/color3.h>

namespace BABYLON {
namespace ProceduralTexturesLibrary {

class WoodProceduralTexture;
using WoodProceduralTexturePtr = std::shared_ptr<WoodProceduralTexture>;

class BABYLON_SHARED_EXPORT WoodProceduralTexture : public ProceduralTexture {

public:
  template <typename... Ts>
  static WoodProceduralTexturePtr New(Ts&&... args)
  {
    auto texture = std::shared_ptr<WoodProceduralTexture>(
      new WoodProceduralTexture(std::forward<Ts>(args)...));
    texture->addToScene(texture);

    return texture;
  }
  ~WoodProceduralTexture(); // = default

  void updateShaderUniforms();

  /**
   * @brief Serializes this wood procedural texture.
   * @returns a serialized wood procedural texture object
   */
  json serialize() const;

  /**
   * @brief Creates a Wood Procedural Texture from parsed wood procedural
   * texture data.
   * @param parsedTexture defines parsed texture data
   * @param scene defines the current scene
   * @param rootUrl defines the root URL containing wood procedural texture
   * information
   * @returns a parsed Wood Procedural Texture
   */
  static std::unique_ptr<WoodProceduralTexture>
  Parse(const json& parsedTexture, Scene* scene, const std::string& rootUrl);

protected:
  WoodProceduralTexture(const std::string& name, const Size& size, Scene* scene,
                        Texture* fallbackTexture = nullptr,
                        bool generateMipMaps     = false);

  float get_ampScale() const;
  void set_ampScale(float value);
  Color3& get_woodColor();
  void set_woodColor(const Color3& value);

public:
  Property<WoodProceduralTexture, float> ampScale;
  Property<WoodProceduralTexture, Color3> woodColor;

private:
  float _ampScale;
  Color3 _woodColor;

}; // end of class WoodProceduralTexture

} // end of namespace ProceduralTexturesLibrary
} // end of namespace BABYLON

#endif // end of
       // BABYLON_PROCEDURAL_TEXTURES_LIBRARY_WOOD_WOOD_PROCEDURAL_TEXTURE_H
