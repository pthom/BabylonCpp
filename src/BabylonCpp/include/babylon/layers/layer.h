#ifndef BABYLON_LAYERS_LAYER_H
#define BABYLON_LAYERS_LAYER_H

#include <babylon/babylon_api.h>
#include <babylon/engines/scene.h>
#include <babylon/maths/color4.h>
#include <babylon/maths/vector2.h>
#include <babylon/misc/observable.h>
#include <babylon/misc/observer.h>

namespace BABYLON {

class Effect;
class Layer;
class Texture;
class VertexBuffer;
class WebGLDataBuffer;
using EffectPtr          = std::shared_ptr<Effect>;
using TexturePtr         = std::shared_ptr<Texture>;
using VertexBufferPtr    = std::shared_ptr<VertexBuffer>;
using WebGLDataBufferPtr = std::shared_ptr<WebGLDataBuffer>;

/**
 * @brief This represents a full screen 2d layer.
 * This can be useful to display a picture in the  background of your scene for instance.
 * @see https://www.babylonjs-playground.com/#08A2BS#1
 */
class BABYLON_SHARED_EXPORT Layer : public std::enable_shared_from_this<Layer> {

  using LayerCallbackType = std::function<void(Layer*, EventState&)>;

public:
  template <typename... Ts>
  static LayerPtr New(Ts&&... args)
  {
    return std::shared_ptr<Layer>(new Layer(std::forward<Ts>(args)...));
  }
  virtual ~Layer(); // = default

  /**
   * @brief Hidden
   */
  void _rebuild();

  /**
   * @brief Renders the layer in the scene.
   */
  void render();

  /**
   * @brief Disposes and releases the associated ressources.
   */
  void dispose();

protected:
  /**
   * @brief Instantiates a new layer.
   * This represents a full screen 2d layer.
   * This can be useful to display a picture in the  background of your scene for instance.
   * @see https://www.babylonjs-playground.com/#08A2BS#1
   * @param name Define the name of the layer in the scene
   * @param imgUrl Define the url of the texture to display in the layer
   * @param scene Define the scene the layer belongs to
   * @param isBackground Defines whether the layer is displayed in front or behind the scene
   * @param color Defines a color for the layer
   */
  Layer(const std::string& name, const std::string& imgUrl, Scene* scene, bool isBackground = true,
        const Color4& color = Color4(1.f, 1.f, 1.f, 1.f));

  // Events
  void set_onDispose(const LayerCallbackType& callback);
  void set_onBeforeRender(const LayerCallbackType& callback);
  void set_onAfterRender(const LayerCallbackType& callback);

private:
  void _createIndexBuffer();

public:
  /**
   * Define the texture the layer should display.
   */
  TexturePtr texture;

  /**
   * Is the layer in background or foreground.
   */
  bool isBackground;

  /**
   * Define the color of the layer (instead of texture).
   */
  Color4 color;

  /**
   * Define the scale of the layer in order to zoom in out of the texture.
   */
  Vector2 scale;

  /**
   * Define an offset for the layer in order to shift the texture.
   */
  Vector2 offset;

  /**
   * Define the alpha blending mode used in the layer in case the texture or color has an alpha.
   */
  unsigned int alphaBlendingMode;

  /**
   * Define if the layer should alpha test or alpha blend with the rest of the scene. Alpha test
   * will not mix with the background color in case of transparency. It will either use the texture
   * color or the background depending on the alpha value of the current pixel.
   */
  bool alphaTest;

  /**
   * Define a mask to restrict the layer to only some of the scene cameras.
   */
  unsigned int layerMask;

  /**
   * Define the list of render target the layer is visible into.
   */
  std::vector<RenderTargetTexturePtr> renderTargetTextures;

  /**
   * Define if the layer is only used in renderTarget or if it also renders in the main frame buffer
   * of the canvas.
   */
  bool renderOnlyInRenderTargetTextures;

  /**
   * An event triggered when the layer is disposed.
   */
  Observable<Layer> onDisposeObservable;

  /**
   * Back compatibility with callback before the onDisposeObservable existed.
   * The set callback will be triggered when the layer has been disposed.
   */
  WriteOnlyProperty<Layer, LayerCallbackType> onDispose;

  /**
   * An event triggered before rendering the scene
   */
  Observable<Layer> onBeforeRenderObservable;

  /**
   * Back compatibility with callback before the onBeforeRenderObservable
   * existed. The set callback will be triggered just before rendering the layer.
   */
  WriteOnlyProperty<Layer, LayerCallbackType> onBeforeRender;

  /**
   * An event triggered after rendering the scene
   */
  Observable<Layer> onAfterRenderObservable;

  /**
   * Back compatibility with callback before the onAfterRenderObservable existed. The set callback
   * will be triggered just after rendering the layer.
   */
  WriteOnlyProperty<Layer, LayerCallbackType> onAfterRender;

private:
  // Events
  Observer<Layer>::Ptr _onDisposeObserver;
  Observer<Layer>::Ptr _onBeforeRenderObserver;
  Observer<Layer>::Ptr _onAfterRenderObserver;
  // Properties
  std::string _name;
  Scene* _scene;
  std::unordered_map<std::string, VertexBufferPtr> _vertexBuffers;
  WebGLDataBufferPtr _indexBuffer;
  EffectPtr _effect;
  EffectPtr _alphaTestEffect;

}; // end of class Layer

} // end of namespace BABYLON

#endif // end of BABYLON_LAYERS_LAYER_H
