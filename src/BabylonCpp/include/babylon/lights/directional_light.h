#ifndef BABYLON_LIGHTS_DIRECTIONAL_LIGHT_H
#define BABYLON_LIGHTS_DIRECTIONAL_LIGHT_H

#include <babylon/babylon_api.h>
#include <babylon/lights/light.h>
#include <babylon/lights/shadow_light.h>

namespace BABYLON {

class DirectionalLight;
using DirectionalLightPtr = std::shared_ptr<DirectionalLight>;

/**
 * @brief A directional light is defined by a direction (what a surprise!).
 * The light is emitted from everywhere in the specified direction, and has an
 * infinite range. An example of a directional light is when a distance planet
 * is lit by the apparently parallel lines of light from its sun. Light in a
 * downward direction will light the top of an object. Documentation:
 * https://doc.babylonjs.com/babylon101/lights
 */
class BABYLON_SHARED_EXPORT DirectionalLight : public ShadowLight {

public:
  static void AddNodeConstructor();

public:
  template <typename... Ts>
  static DirectionalLightPtr New(Ts&&... args)
  {
    auto light = std::shared_ptr<DirectionalLight>(
      new DirectionalLight(std::forward<Ts>(args)...));
    light->addToScene(light);

    return light;
  }
  ~DirectionalLight(); // = default

  Type type() const override;

  /**
   * @brief Returns the string "DirectionalLight".
   * @return The class name
   */
  const std::string getClassName() const override;

  /**
   * @brief Returns the integer 1.
   * @return The light Type id as a constant defines in Light.LIGHTTYPEID_x
   */
  unsigned int getTypeID() const override;

  /**
   * @brief Sets the passed Effect object with the DirectionalLight transformed
   * position (or position if not parented) and the passed name.
   * @param effect The effect to update
   * @param lightIndex The index of the light in the effect to update
   * @returns The directional light
   */
  void transferToEffect(const EffectPtr& effect,
                        const std::string& lightIndex) override;

  /**
   * @brief Sets the passed Effect "effect" with the Light information.
   * @param effect The effect to update
   * @param lightDataUniformName The uniform used to store light data (position
   * or direction)
   * @returns The light
   */
  DirectionalLight& transferToNodeMaterialEffect(
    const EffectPtr& effect, const std::string& lightDataUniformName) override;

  /**
   * @brief Gets the minZ used for shadow according to both the scene and the
   * light. Values are fixed on directional lights as it relies on an ortho
   * projection hence the need to convert being -1 and 1 to 0 and 1 doing (depth
   * + min) / (min + max) -> (depth + 1) / (1 + 1) -> (depth * 0.5) + 0.5.
   * @param activeCamera The camera we are returning the min for
   * @returns the depth min z
   */
  float getDepthMinZ(const Camera& activeCamera) const override;

  /**
   * @brief Gets the maxZ used for shadow according to both the scene and the
   * light. Values are fixed on directional lights as it relies on an ortho
   * projection hence the need to convert being -1 and 1 to 0 and 1 doing (depth
   * + min) / (min + max) -> (depth + 1) / (1 + 1) -> (depth * 0.5) + 0.5.
   * @param activeCamera The camera we are returning the max for
   * @returns the depth max z
   */
  float getDepthMaxZ(const Camera& activeCamera) const override;

  /**
   * @brief Prepares the list of defines specific to the light type.
   * @param defines the list of defines
   * @param lightIndex defines the index of the light for the effect
   */
  void prepareLightSpecificDefines(MaterialDefines& defines,
                                   unsigned int lightIndex) override;

protected:
  /**
   * @brief Creates a DirectionalLight object in the scene, oriented towards the
   * passed direction (Vector3). The directional light is emitted from
   * everywhere in the given direction. It can cast shadows.
   * Documentation : https://doc.babylonjs.com/babylon101/lights
   * @param name The friendly name of the light
   * @param direction The direction of the light
   * @param scene The scene the light belongs to
   */
  DirectionalLight(const std::string& name, const Vector3& direction,
                   Scene* scene);

  /**
   * @brief Sets the passed matrix "matrix" as projection matrix for the shadows
   * cast by the light according to the passed view matrix.
   * @returns The DirectionalLight Shadow projection matrix.
   */
  void _setDefaultShadowProjectionMatrix(
    Matrix& matrix, const Matrix& viewMatrix,
    const std::vector<AbstractMesh*>& renderList) override;

  /**
   * @brief Sets the passed matrix "matrix" as fixed frustum projection matrix
   * for the shadows cast by the light according to the passed view matrix.
   * @returns The DirectionalLight Shadow projection matrix.
   */
  void _setDefaultFixedFrustumShadowProjectionMatrix(Matrix& matrix);

  /**
   * @brief Sets the passed matrix "matrix" as auto extend projection matrix for
   * the shadows cast by the light according to the passed view matrix.
   * @returns The DirectionalLight Shadow projection matrix.
   */
  void _setDefaultAutoExtendShadowProjectionMatrix(
    Matrix& matrix, const Matrix& viewMatrix,
    const std::vector<AbstractMesh*>& renderList);

  void _buildUniformLayout() override;

private:
  /**
   * @brief Fix frustum size for the shadow generation. This is disabled if the
   * value is 0.
   */
  float get_shadowFrustumSize() const;

  /**
   * @brief Specifies a fix frustum size for the shadow generation.
   */
  void set_shadowFrustumSize(float value);

  /**
   * @brief Gets the shadow projection scale against the optimal computed one.
   * 0.1 by default which means that the projection window is increase by 10%
   * from the optimal size. This does not impact in fixed frustum size
   * (shadowFrustumSize being set)
   */
  float get_shadowOrthoScale() const;

  /**
   * @brief Sets the shadow projection scale against the optimal computed one.
   * 0.1 by default which means that the projection window is increase by 10%
   * from the optimal size. This does not impact in fixed frustum size
   * (shadowFrustumSize being set)
   */
  void set_shadowOrthoScale(float value);

public:
  /**
   * Frustum size for the shadow generation.
   */
  Property<DirectionalLight, float> shadowFrustumSize;

  /**
   * Shadow projection scale against the optimal computed one.
   */
  Property<DirectionalLight, float> shadowOrthoScale;

  /**
   * Automatically compute the projection matrix to best fit (including all the
   * casters) on each frame.
   */
  bool autoUpdateExtends;

private:
  float _shadowFrustumSize;
  float _shadowOrthoScale;
  // Cache
  float _orthoLeft;
  float _orthoRight;
  float _orthoTop;
  float _orthoBottom;

  static bool NodeConstructorAdded;

}; // end of class DirectionalLight

} // end of namespace BABYLON

#endif // end of BABYLON_LIGHTS_DIRECTIONAL_LIGHT_H
