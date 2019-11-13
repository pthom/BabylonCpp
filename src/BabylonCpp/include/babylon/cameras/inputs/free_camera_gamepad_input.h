#ifndef BABYLON_CAMERAS_INPUTS_FREE_CAMERA_GAMEPAD_INPUT_H
#define BABYLON_CAMERAS_INPUTS_FREE_CAMERA_GAMEPAD_INPUT_H

#include <babylon/babylon_api.h>
#include <babylon/cameras/free_camera.h>
#include <babylon/cameras/icamera_input.h>
#include <babylon/core/structs.h>
#include <babylon/events/pointer_info.h>
#include <babylon/misc/observer.h>

namespace BABYLON {

class Gamepad;

/**
 * @brief Manage the gamepad inputs to control a free camera.
 * @see http://doc.babylonjs.com/how_to/customizing_camera_inputs
 */
class BABYLON_SHARED_EXPORT FreeCameraGamepadInput
    : public ICameraInput<FreeCamera> {

public:
  /**
   * @brief Instantiate the input.
   */
  FreeCameraGamepadInput();
  virtual ~FreeCameraGamepadInput(); // = default

  /**
   * @brief Attach the input controls to a specific dom element to get the input
   * from.
   * @param element Defines the element the controls should be listened from
   * @param noPreventDefault Defines whether event caught by the controls should
   * call preventdefault()
   * (https://developer.mozilla.org/en-US/docs/Web/API/Event/preventDefault)
   */
  void attachControl(ICanvas* canvas, bool noPreventDefault) override;

  /**
   * @brief Detach the current controls from the specified dom element.
   * @param element Defines the element to stop listening the inputs from
   */
  void detachControl(ICanvas* canvas) override;

  /**
   * @brief Update the current camera state depending on the inputs that have
   * been used this frame. This is a dynamically created lambda to avoid the
   * performance penalty of looping for inputs in the render loop.
   */
  void checkInputs() override;

  /**
   * @brief Gets the class name of the current intput.
   * @returns the class name
   */
  const std::string getClassName() const override;

  /**
   * @brief Get the friendly name associated with the input class.
   * @returns the input friendly name
   */
  const std::string getSimpleName() const override;

public:
  /**
   * Define the Gamepad controlling the input
   */
  Gamepad* gamepad;

  /**
   * Defines the gamepad rotation sensiblity.
   * This is the threshold from when rotation starts to be accounted for to
   * prevent jittering.
   */
  float gamepadAngularSensibility;

  /**
   * Defines the gamepad move sensiblity.
   * This is the threshold from when moving starts to be accounted for for to
   * prevent jittering.
   */
  float gamepadMoveSensibility;

private:
  Observer<Gamepad>::Ptr _onGamepadConnectedObserver;
  Observer<Gamepad>::Ptr _onGamepadDisconnectedObserver;

  Matrix _cameraTransform;
  Vector3 _deltaTransform;
  Vector3 _vector3;
  Vector2 _vector2;

}; // end of class FreeCameraGamepadInput

} // end of namespace BABYLON

#endif // end of BABYLON_CAMERAS_INPUTS_FREE_CAMERA_GAMEPAD_INPUT_H
