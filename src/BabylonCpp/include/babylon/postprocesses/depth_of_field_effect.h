#ifndef BABYLON_POSTPROCESSES_DEPTH_OF_FIELD_EFFECT_H
#define BABYLON_POSTPROCESSES_DEPTH_OF_FIELD_EFFECT_H

#include <memory>

#include <babylon/babylon_api.h>
#include <babylon/postprocesses/depth_of_field_effect_blur_level.h>
#include <babylon/postprocesses/renderpipeline/post_process_render_effect.h>

namespace BABYLON {

class CircleOfConfusionPostProcess;
class DepthOfFieldBlurPostProcess;
class DepthOfFieldEffect;
class DepthOfFieldMergePostProcess;
class PassPostProcess;
class RenderTargetTexture;
class Scene;
using CircleOfConfusionPostProcessPtr
  = std::shared_ptr<CircleOfConfusionPostProcess>;
using DepthOfFieldBlurPostProcessPtr
  = std::shared_ptr<DepthOfFieldBlurPostProcess>;
using DepthOfFieldEffectPtr = std::shared_ptr<DepthOfFieldEffect>;
using DepthOfFieldMergePostProcessPtr
  = std::shared_ptr<DepthOfFieldMergePostProcess>;
using RenderTargetTexturePtr = std::shared_ptr<RenderTargetTexture>;
using PassPostProcessPtr     = std::shared_ptr<PassPostProcess>;

/**
 * @brief The depth of field effect applies a blur to objects that are closer or
 * further from where the camera is focusing.
 */
class BABYLON_SHARED_EXPORT DepthOfFieldEffect
    : public PostProcessRenderEffect {

public:
  template <typename... Ts>
  static DepthOfFieldEffectPtr New(Ts&&... args)
  {
    return std::shared_ptr<DepthOfFieldEffect>(
      new DepthOfFieldEffect(std::forward<Ts>(args)...));
  }
  ~DepthOfFieldEffect(); // = default

  /**
   * @brief Get the current class name of the current effet.
   * @returns "DepthOfFieldEffect"
   */
  std::string getClassName() const;

  /**
   * @brief Disposes each of the internal effects for a given camera.
   * @param camera The camera to dispose the effect on.
   */
  void disposeEffects(Camera* camera);

  /**
   * @brief Hidden Internal
   */
  void _updateEffects();

  /**
   * @brief Internal
   * @returns if all the contained post processes are ready.
   * Hidden
   */
  bool _isReady() const;

protected:
  /**
   * @brief  Creates a new instance DepthOfFieldEffect.
   * @param scene The scene the effect belongs to.
   * @param depthTexture The depth texture of the scene to compute the circle of
   * confusion.This must be set in order for this to function but may be set
   * after initialization if needed.
   * @param pipelineTextureType The type of texture to be used when performing
   * the post processing.
   * @param blockCompilation If compilation of the shader should not be done in
   * the constructor. The updateEffect method can be used to compile the shader
   * at a later time. (default: false)
   */
  DepthOfFieldEffect(Scene* scene, RenderTargetTexture* depthTexture,
                     DepthOfFieldEffectBlurLevel blurLevel
                     = DepthOfFieldEffectBlurLevel::Low,
                     unsigned int pipelineTextureType = 0,
                     bool blockCompilation            = false);

  /**
   * @brief The focal the length of the camera used in the effect in scene
   * units/1000 (eg. millimeter).
   */
  void set_focalLength(float value);
  float get_focalLength() const;

  /**
   * @brief F-Stop of the effect's camera. The diameter of the resulting
   * aperture can be computed by lensSize/fStop. (default: 1.4).
   */
  void set_fStop(float value);
  float get_fStop() const;

  /**
   * @brief Distance away from the camera to focus on in scene units/1000 (eg.
   * millimeter). (default: 2000).
   */
  void set_focusDistance(float value);
  float get_focusDistance() const;

  /**
   * @brief Max lens size in scene units/1000 (eg. millimeter). Standard cameras
   * are 50mm. (default: 50) The diamater of the resulting aperture can be
   * computed by lensSize/fStop.
   */
  void set_lensSize(float value);
  float get_lensSize() const;

  /**
   * @brief Depth texture to be used to compute the circle of confusion. This
   * must be set here or in the constructor in order for the post process to
   * function.
   */
  void set_depthTexture(const RenderTargetTexturePtr& value);

public:
  /**
   * Hidden Internal, blurs from high to low
   */
  std::vector<DepthOfFieldBlurPostProcessPtr> _depthOfFieldBlurX;

  /**
   * Hidden Internal post processes in depth of field effect
   */
  std::vector<PostProcessPtr> _effects;

  /**
   * The focal the length of the camera used in the effect
   */
  Property<DepthOfFieldEffect, float> focalLength;

  /**
   * F-Stop of the effect's camera. The diameter of the resulting aperture can
   * be computed by lensSize/fStop. (default: 1.4)
   */
  Property<DepthOfFieldEffect, float> fStop;

  /**
   * Distance away from the camera to focus on in scene units/1000 (eg.
   * millimeter). (default: 2000)
   */
  Property<DepthOfFieldEffect, float> focusDistance;

  /**
   * Max lens size in scene units/1000 (eg. millimeter). Standard cameras are
   * 50mm. (default: 50) The diamater of the resulting aperture can be computed
   * by lensSize/fStop.
   */
  Property<DepthOfFieldEffect, float> lensSize;

  /**
   * Depth texture to be used to compute the circle of confusion. This must be
   * set here or in the constructor in order for the post process to function.
   */
  WriteOnlyProperty<DepthOfFieldEffect, RenderTargetTexturePtr> depthTexture;

private:
  DepthOfFieldMergePostProcessPtr _dofMerge;
  PassPostProcessPtr _depthOfFieldPass;
  CircleOfConfusionPostProcessPtr _circleOfConfusion;
  std::vector<DepthOfFieldBlurPostProcessPtr> _depthOfFieldBlurY;

}; // end of class ConvolutionPostProcess

} // end of namespace BABYLON

#endif // end of BABYLON_POSTPROCESSES_DEPTH_OF_FIELD_EFFECT_H
