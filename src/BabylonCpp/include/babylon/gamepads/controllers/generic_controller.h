#ifndef BABYLON_GAMEPADS_CONTROLLERS_GENERIC_CONTROLLER_H
#define BABYLON_GAMEPADS_CONTROLLERS_GENERIC_CONTROLLER_H

#include <babylon/babylon_api.h>
#include <babylon/gamepads/controllers/_game_pad_factory.h>
#include <babylon/gamepads/controllers/web_vr_controller.h>

namespace BABYLON {

class GenericController;
class IBrowserGamepad;
using GenericControllerPtr = std::shared_ptr<GenericController>;
using IBrowserGamepadPtr   = std::shared_ptr<IBrowserGamepad>;

/**
 * @brief Generic Controller factory.
 */
struct GenericControllerFactory : public _GamePadFactory {
  /**
   * @brief Returns wether or not the current gamepad can be created for this
   * type of controller.
   * @param gamepadInfo Defines the gamepad info as receveid from the controller
   * APIs.
   * @returns true if it can be created, otherwise false
   */
  bool canCreate(const IBrowserGamepadPtr& gamepadInfo) const override;

  /**
   * @brief Creates a new instance of the Gamepad.
   * @param gamepadInfo Defines the gamepad info as receveid from the controller
   * APIs.
   * @returns the new gamepad instance
   */
  WebVRControllerPtr
  create(const IBrowserGamepadPtr& gamepadInfo) const override;
}; // end of struct GenericControllerFactory

/**
 *@brief  Generic Controller.
 */
class BABYLON_SHARED_EXPORT GenericController : public WebVRController {

private:
  /**
   * Base Url for the controller model.
   */
  static constexpr const char* MODEL_BASE_URL
    = "https://controllers.babylonjs.com/generic/";
  /**
   * File name for the controller model.
   */
  static constexpr const char* MODEL_FILENAME = "generic.babylon";

public:
  template <typename... Ts>
  static GenericControllerPtr New(Ts&&... args)
  {
    return std::shared_ptr<GenericController>(
      new GenericController(std::forward<Ts>(args)...));
  }
  ~GenericController(); // = default

  /**
   * @brief Implements abstract method on WebVRController class, loading
   * controller meshes and calling this.attachToMesh if successful.
   * @param scene scene in which to add meshes
   * @param meshLoaded optional callback function that will be called if the
   * mesh loads successfully.
   */
  void initControllerMesh(
    Scene* scene,
    const std::function<void(AbstractMesh* mesh)>& meshLoaded) override;

protected:
  /**
   * @brief Creates a new GenericController from a gamepad.
   * @param vrGamepad the gamepad that the controller should be created from
   */
  GenericController(const IBrowserGamepadPtr& vrGamepad);

  /**
   * @brief Called once for each button that changed state since the last frame.
   * @param buttonIdx Which button index changed
   * @param state New state of the button
   * @param changes Which properties on the state changed since last frame
   */
  void _handleButtonChange(unsigned int buttonIdx,
                           const ExtendedGamepadButton& state,
                           const GamepadButtonChanges& changes) override;

}; // end of class GenericController

} // end of namespace BABYLON

#endif // end of BABYLON_GAMEPADS_CONTROLLERS_GENERIC_CONTROLLER_H
