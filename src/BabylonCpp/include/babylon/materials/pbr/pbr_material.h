#ifndef BABYLON_MATERIALS_PBR_PBR_MATERIAL_H
#define BABYLON_MATERIALS_PBR_PBR_MATERIAL_H

#include <babylon/babylon_global.h>
#include <babylon/materials/pbr/pbr_base_material.h>

namespace BABYLON {

/**
 * @brief The Physically based material of BJS.
 *
 * This offers the main features of a standard PBR material.
 * For more information, please refer to the documentation :
 * http://doc.babylonjs.com/extensions/Physically_Based_Rendering
 */
class BABYLON_SHARED_EXPORT PBRMaterial : public PBRBaseMaterial {

private:
  /**
   * No transparency mode, Alpha channel is not use.
   */
  static constexpr unsigned int _PBRMATERIAL_OPAQUE = 0;

  /**
   * Alpha Test mode, pixel are discarded below a certain threshold defined by
   * the alpha cutoff value.
   */
  static constexpr unsigned int _PBRMATERIAL_ALPHATEST = 1;

  /**
   * Represents the value for Alpha Blend.  Pixels are blended (according to the
   * alpha mode) with the already drawn pixels in the current frame buffer.
   */
  static constexpr unsigned int _PBRMATERIAL_ALPHABLEND = 2;

  /**
   * Represents the value for Alpha Test and Blend.  Pixels are blended
   * (according to the alpha mode) with the already drawn pixels in the current
   * frame buffer. They are also discarded below the alpha cutoff threshold to
   * improve performances.
   */
  static constexpr unsigned int _PBRMATERIAL_ALPHATESTANDBLEND = 3;

public:
  /**
   * @brief PBRMaterialTransparencyMode: No transparency mode, Alpha channel is
   * not use.
   */
  static constexpr unsigned int PBRMATERIAL_OPAQUE()
  {
    return PBRMaterial::_PBRMATERIAL_OPAQUE;
  }

  /**
   * @brief PBRMaterialTransparencyMode: Alpha Test mode, pixel are discarded
   * below a certain threshold defined by the alpha cutoff value.
   */
  static constexpr unsigned int PBRMATERIAL_ALPHATEST()
  {
    return PBRMaterial::_PBRMATERIAL_ALPHATEST;
  }

  /**
   * @brief PBRMaterialTransparencyMode: Pixels are blended (according to the
   * alpha mode) with the already drawn pixels in the current frame buffer.
   */
  static constexpr unsigned int PBRMATERIAL_ALPHABLEND()
  {
    return PBRMaterial::_PBRMATERIAL_ALPHABLEND;
  }

  /**
   * @brief PBRMaterialTransparencyMode: Pixels are blended (according to the
   * alpha mode) with the already drawn pixels in the current frame buffer. They
   * are also discarded below the alpha cutoff threshold to improve
   * performances.
   */
  static constexpr unsigned int PBRMATERIAL_ALPHATESTANDBLEND()
  {
    return PBRMaterial::_PBRMATERIAL_ALPHATESTANDBLEND;
  }

public:
  /**
   * @brief Instantiates a new PBRMaterial instance.
   *
   * @param name The material name
   * @param scene The scene the material will be use in.
   */
  PBRMaterial(const string_t& name, Scene* scene);
  ~PBRMaterial() override;

  /**
   * @brief Returns the name of this material class.
   */
  const char* getClassName() const override;

  /**
   * @brief Returns an array of the actively used textures.
   * @returns - Array of BaseTextures
   */
  vector_t<BaseTexture*> getActiveTextures() const override;

  /**
   * @brief Checks to see if a texture is used in the material.
   * @param texture - Base texture to use.
   * @returns - Boolean specifying if a texture is used in the material.
   */
  bool hasTexture(BaseTexture* texture) const override;

  /**
   * @brief Makes a duplicate of the current material.
   * @param name - name to use for the new material.
   */
  PBRMaterial* clone(const string_t& name,
                     bool cloneChildren = false) const override;

  /**
   * @brief Serializes this PBR Material.
   * @returns - An object with the serialized material.
   */
  Json::object serialize() const;

  // Statics
  /**
   * @brief Parses a PBR Material from a serialized object.
   * @param source - Serialized object.
   * @param scene - BJS scene instance.
   * @param rootUrl - url for the scene object
   * @returns - PBRMaterial
   */
  static PBRMaterial* Parse(const Json::value& source, Scene* scene,
                            const string_t& rootUrl);

  // Getters / Setters

  /**
   * @brief Gets the image processing configuration used either in this
   * material.
   */
  ImageProcessingConfiguration* imageProcessingConfiguration();
  const ImageProcessingConfiguration* imageProcessingConfiguration() const;

  /**
   * @brief Sets the Default image processing configuration used either in the
   * this material.
   *
   * If sets to null, the scene one is in use.
   */
  void setImageProcessingConfiguration(ImageProcessingConfiguration* value);

  /**
   * @brief Gets wether the color curves effect is enabled.
   */
  bool cameraColorCurvesEnabled() const;

  /**
   * @brief Sets wether the color curves effect is enabled.
   */
  void setCameraColorCurvesEnabled(bool value);

  /**
   * @brief Gets wether the color grading effect is enabled.
   */
  bool cameraColorGradingEnabled() const;

  /**
   * @brief Gets wether the color grading effect is enabled.
   */
  void setCameraColorGradingEnabled(bool value);

  /**
   * @brief Gets wether tonemapping is enabled or not.
   */
  bool cameraToneMappingEnabled() const;

  /**
   * Sets wether tonemapping is enabled or not
   */
  void setCameraToneMappingEnabled(bool value);

  /**
   * @brief The camera exposure used on this material.
   * This property is here and not in the camera to allow controlling exposure
   * without full screen post process.
   * This corresponds to a photographic exposure.
   */
  float cameraExposure() const;

  /**
   * @brief The camera exposure used on this material.
   * This property is here and not in the camera to allow controlling exposure
   * without full screen post process.
   * This corresponds to a photographic exposure.
   */
  void setCameraExposure(float value);

  /**
   * @brief Gets The camera contrast used on this material.
   */
  float cameraContrast() const;

  /**
   * @brief Sets The camera contrast used on this material.
   */
  void setCameraContrast(float value);

  /**
   * @brief Gets the Color Grading 2D Lookup Texture.
   */
  BaseTexture* cameraColorGradingTexture() const;

  /**
   * @brief Sets the Color Grading 2D Lookup Texture.
   */
  void setCameraColorGradingTexture(BaseTexture* value);

  /**
   * @brief The color grading curves provide additional color adjustmnent that
   * is applied after any color grading transform (3D LUT).
   * They allow basic adjustment of saturation and small exposure adjustments,
   * along with color filter tinting to provide white balance adjustment or more
   * stylistic effects.
   * These are similar to controls found in many professional imaging or
   * colorist software. The global controls are applied to the entire image. For
   * advanced tuning, extra controls are provided to adjust the shadow, midtone
   * and highlight areas of the image;
   * corresponding to low luminance, medium luminance, and high luminance areas
   * respectively.
   */
  ColorCurves* cameraColorCurves();

  /**
   * @brief The color grading curves provide additional color adjustmnent that
   * is applied after any color grading transform (3D LUT).
   * They allow basic adjustment of saturation and small exposure adjustments,
   * along with color filter tinting to provide white balance adjustment or more
   * stylistic effects.
   * These are similar to controls found in many professional imaging or
   * colorist software. The global controls are applied to the entire image. For
   * advanced tuning, extra controls are provided to adjust the shadow, midtone
   * and highlight areas of the image;
   * corresponding to low luminance, medium luminance, and high luminance areas
   * respectively.
   */
  void setCameraColorCurves(ColorCurves* value);

public:
  /**
   * Intensity of the direct lights e.g. the four lights available in your
   * scene.
   * This impacts both the direct diffuse and specular highlights.
   */
  float directIntensity;

  /**
   * Intensity of the emissive part of the material.
   * This helps controlling the emissive effect without modifying the emissive
   * color.
   */
  float emissiveIntensity;

  /**
   * Intensity of the environment e.g. how much the environment will light the
   * object
   * either through harmonics for rough material or through the refelction for
   * shiny ones.
   */
  float environmentIntensity;

  /**
   * This is a special control allowing the reduction of the specular highlights
   * coming from the
   * four lights of the scene. Those highlights may not be needed in full
   * environment lighting.
   */
  float specularIntensity;

  /**
   * Debug Control allowing disabling the bump map on this material.
   */
  bool disableBumpMap;

  /**
   * AKA Diffuse Texture in standard nomenclature.
   */
  BaseTexture* albedoTexture;

  /**
   * AKA Occlusion Texture in other nomenclature.
   */
  BaseTexture* ambientTexture;

  /**
   * AKA Occlusion Texture Intensity in other nomenclature.
   */
  float ambientTextureStrength;

  /**
   * Stores the alpha values in a texture.
   */
  BaseTexture* opacityTexture;

  /**
   * Stores the reflection values in a texture.
   */
  BaseTexture* reflectionTexture;

  /**
   * Stores the emissive values in a texture.
   */
  BaseTexture* emissiveTexture;

  /**
   * AKA Specular texture in other nomenclature.
   */
  BaseTexture* reflectivityTexture;

  /**
   * Used to switch from specular/glossiness to metallic/roughness workflow.
   */
  BaseTexture* metallicTexture;

  /**
   * Specifies the metallic scalar of the metallic/roughness workflow.
   * Can also be used to scale the metalness values of the metallic texture.
   */
  float metallic;

  /**
   * Specifies the roughness scalar of the metallic/roughness workflow.
   * Can also be used to scale the roughness values of the metallic texture.
   */
  float roughness;

  /**
   * Used to enable roughness/glossiness fetch from a separate chanel depending
   * on the current mode.
   * Gray Scale represents roughness in metallic mode and glossiness in specular
   * mode.
   */
  BaseTexture* microSurfaceTexture;

  /**
   * Stores surface normal data used to displace a mesh in a texture.
   */
  BaseTexture* bumpTexture;

  /**
   * Stores the pre-calculated light information of a mesh in a texture.
   */
  BaseTexture* lightmapTexture;

  /**
   * Stores the refracted light information in a texture.
   */
  BaseTexture* refractionTexture;

  /**
   * The color of a material in ambient lighting.
   */
  Color3 ambientColor;

  /**
   * AKA Diffuse Color in other nomenclature.
   */
  Color3 albedoColor;

  /**
   * AKA Specular Color in other nomenclature.
   */
  Color3 reflectivityColor;

  /**
   * The color reflected from the material.
   */
  Color3 reflectionColor;

  /**
   * The color emitted from the material.
   */
  Color3 emissiveColor;

  /**
   * AKA Glossiness in other nomenclature.
   */
  float microSurface;

  /**
   * source material index of refraction (IOR)' / 'destination material IOR.
   */
  float indexOfRefraction;

  /**
   * Controls if refraction needs to be inverted on Y. This could be usefull for
   * procedural texture.
   */
  bool invertRefractionY;

  /**
   * This parameters will make the material used its opacity to control how much
   * it is refracting aginst not.
   * Materials half opaque for instance using refraction could benefit from this
   * control.
   */
  bool linkRefractionWithTransparency;

  bool useLightmapAsShadowmap;

  /**
   * Specifies that the alpha is coming form the albedo channel alpha channel
   * for alpha blending.
   */
  bool useAlphaFromAlbedoTexture;

  /**
   * Enforces alpha test in opaque or blend mode in order to improve the
   * performances of some situations.
   */
  bool forceAlphaTest;

  /**
   * Defines the alpha limits in alpha test mode.
   */
  float alphaCutOff;

  /**
   * Specifies that the material will keeps the specular highlights over a
   * transparent surface (only the most limunous ones).
   * A car glass is a good exemple of that. When sun reflects on it you can not
   * see what is behind.
   */
  bool useSpecularOverAlpha;

  /**
   * Specifies if the reflectivity texture contains the glossiness information
   * in its alpha channel.
   */
  bool useMicroSurfaceFromReflectivityMapAlpha;

  /**
   * Specifies if the metallic texture contains the roughness information in its
   * alpha channel.
   */
  bool useRoughnessFromMetallicTextureAlpha;

  /**
   * Specifies if the metallic texture contains the roughness information in its
   * green channel.
   */
  bool useRoughnessFromMetallicTextureGreen;

  /**
   * Specifies if the metallic texture contains the metallness information in
   * its blue channel.
   */
  bool useMetallnessFromMetallicTextureBlue;

  /**
   * Specifies if the metallic texture contains the ambient occlusion
   * information in its red channel.
   */
  bool useAmbientOcclusionFromMetallicTextureRed;

  /**
   * Specifies if the ambient texture contains the ambient occlusion information
   * in its red channel only.
   */
  bool useAmbientInGrayScale;

  /**
   * In case the reflectivity map does not contain the microsurface information
   * in its alpha channel,
   * The material will try to infer what glossiness each pixel should be.
   */
  bool useAutoMicroSurfaceFromReflectivityMap;

  /**
   * BJS is using an harcoded light falloff based on a manually sets up range.
   * In PBR, one way to represents the fallof is to use the inverse squared root
   * algorythm.
   * This parameter can help you switch back to the BJS mode in order to create
   * scenes using both materials.
   */
  bool usePhysicalLightFalloff;

  /**
   * Specifies that the material will keeps the reflection highlights over a
   * transparent surface (only the most limunous ones).
   * A car glass is a good exemple of that. When the street lights reflects on
   * it you can not see what is behind.
   */
  bool useRadianceOverAlpha;

  /**
   * Allows using an object space normal map (instead of tangent space).
   */
  bool useObjectSpaceNormalMap;

  /**
   * Allows using the bump map in parallax mode.
   */
  bool useParallax;

  /**
   * Allows using the bump map in parallax occlusion mode.
   */
  bool useParallaxOcclusion;

  /**
   * Controls the scale bias of the parallax mode.
   */
  float parallaxScaleBias;

  /**
   * If sets to true, disables all the lights affecting the material.
   */
  bool disableLighting;

  /**
   * Force the shader to compute irradiance in the fragment shader in order to
   * take bump in account.
   */
  bool forceIrradianceInFragment;

  /**
   * Number of Simultaneous lights allowed on the material.
   */
  unsigned int maxSimultaneousLights;

  /**
   * If sets to true, x component of normal map value will invert (x = 1.0 - x).
   */
  bool invertNormalMapX;

  /**
   * If sets to true, y component of normal map value will invert (y = 1.0 - y).
   */
  bool invertNormalMapY;

  /**
   * If sets to true and backfaceCulling is false, normals will be flipped on
   * the backside.
   */
  bool twoSidedLighting;

  /**
   * Specifies that the alpha is premultiplied before output (this enables alpha
   * premultiplied blending).
   * in your scene composition.
   */
  bool preMultiplyAlpha;

  /**
   * A fresnel is applied to the alpha of the model to ensure grazing angles
   * edges are not alpha tested.
   * And/Or occlude the blended part. (alpha is converted to gamma to compute
   * the fresnel)
   */
  bool useAlphaFresnel;

  /**
   * A fresnel is applied to the alpha of the model to ensure grazing angles
   * edges are not alpha tested.
   * And/Or occlude the blended part. (alpha stays linear to compute the
   * fresnel)
   */
  bool useLinearAlphaFresnel;

  /**
   * A fresnel is applied to the alpha of the model to ensure grazing angles
   * edges are not alpha tested.
   * And/Or occlude the blended part.
   */
  BaseTexture* environmentBRDFTexture;

  /**
   * Force normal to face away from face.
   */
  bool forceNormalForward;

  /**
   * Enables specular anti aliasing in the PBR shader.
   * It will both interacts on the Geometry for analytical and IBL lighting.
   * It also prefilter the roughness map based on the bump values.
   */
  bool enableSpecularAntiAliasing;

  /**
   * This parameters will enable/disable Horizon occlusion to prevent normal
   * maps to look shiny when the normal
   * makes the reflect vector face the model (under horizon).
   */
  bool useHorizonOcclusion;

  /**
   * This parameters will enable/disable radiance occlusion by preventing the
   * radiance to lit
   * too much the area relying on ambient texture to define their ambient
   * occlusion.
   */
  bool useRadianceOcclusion;

  /**
   * If set to true, no lighting calculations will be applied.
   */
  bool unlit;

}; // end of class PBRMaterial

} // end of namespace BABYLON

#endif // end of BABYLON_MATERIALS_PBR_PBR_MATERIAL_H
