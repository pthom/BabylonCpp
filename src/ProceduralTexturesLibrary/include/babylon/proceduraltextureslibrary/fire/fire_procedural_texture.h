#ifndef BABYLON_PROCEDURAL_TEXTURES_LIBRARY_FIRE_FIRE_PROCEDURAL_TEXTURE_H
#define BABYLON_PROCEDURAL_TEXTURES_LIBRARY_FIRE_FIRE_PROCEDURAL_TEXTURE_H

#include <babylon/babylon_api.h>
#include <babylon/materials/textures/procedurals/procedural_texture.h>
#include <babylon/math/color3.h>
#include <babylon/math/vector2.h>

namespace BABYLON {
namespace ProceduralTexturesLibrary {

class FireProceduralTexture;
using FireProceduralTexturePtr = std::shared_ptr<FireProceduralTexture>;

class BABYLON_SHARED_EXPORT FireProceduralTexture : public ProceduralTexture {

public:
  template <typename... Ts>
  static FireProceduralTexturePtr New(Ts&&... args)
  {
    auto texture = std::shared_ptr<FireProceduralTexture>(
      new FireProceduralTexture(std::forward<Ts>(args)...));
    texture->addToScene(texture);

    return texture;
  }
  ~FireProceduralTexture(); // = default

  void updateShaderUniforms();

  void render(bool useCameraPostProcess);
  static std::vector<Color3> PurpleFireColors();
  static std::vector<Color3> GreenFireColors();
  static std::vector<Color3> RedFireColors();
  static std::vector<Color3> BlueFireColors();

  /**
   * @brief Serializes this fire procedural texture.
   * @returns a serialized fire procedural texture object
   */
  json serialize() const;

  /**
   * @brief Creates a Fire Procedural Texture from parsed fire procedural
   * texture data.
   * @param parsedTexture defines parsed texture data
   * @param scene defines the current scene
   * @param rootUrl defines the root URL containing fire procedural texture
   * information
   * @returns a parsed Fire Procedural Texture
   */
  static std::unique_ptr<FireProceduralTexture>
  Parse(const json& parsedTexture, Scene* scene, const std::string& rootUrl);

protected:
  FireProceduralTexture(const std::string& name, const Size& size, Scene* scene,
                        Texture* fallbackTexture = nullptr,
                        bool generateMipMaps     = false);

  bool get_autoGenerateTime() const;
  void set_autoGenerateTime(bool value);
  std::vector<Color3>& get_fireColors();
  void set_fireColors(const std::vector<Color3>& value);
  float get_time() const;
  void set_time(float value);
  Vector2& get_speed();
  void set_speed(const Vector2& value);
  float get_alphaThreshold() const;
  void set_alphaThreshold(float value);

public:
  Property<FireProceduralTexture, bool> autoGenerateTime;
  Property<FireProceduralTexture, std::vector<Color3>> fireColors;
  Property<FireProceduralTexture, float> time;
  Property<FireProceduralTexture, Vector2> speed;
  Property<FireProceduralTexture, float> alphaThreshold;

private:
  float _time;
  Vector2 _speed;
  bool _autoGenerateTime;
  std::vector<Color3> _fireColors;
  float _alphaThreshold;

}; // end of class FireProceduralTexture

} // end of namespace ProceduralTexturesLibrary
} // end of namespace BABYLON

#endif // end of
       // BABYLON_PROCEDURAL_TEXTURES_LIBRARY_FIRE_FIRE_PROCEDURAL_TEXTURE_H
