#ifndef BABYLON_CAMERAS_CAMERA_INPUTS_MANAGER_H
#define BABYLON_CAMERAS_CAMERA_INPUTS_MANAGER_H

#include <babylon/babylon_global.h>
#include <babylon/cameras/icamera_input.h>

namespace BABYLON {

template <class TCamera>
class BABYLON_SHARED_EXPORT CameraInputsManager {

public:
  CameraInputsManager();
  CameraInputsManager(TCamera* iCamera);
  ~CameraInputsManager();

  /**
   * Add an input method to a camera.
   * @see http://doc.babylonjs.com/how_to/customizing_camera_inputs
   * @param input camera input method
   */
  void add(shared_ptr_t<ICameraInput<TCamera>>&& input);

  /**
   * @brief Remove a specific input method from a camera
   * example: camera.inputs.remove(camera.inputs.attached.mouse);
   * @param inputToRemove camera input method
   */
  void remove(ICameraInput<TCamera>* inputToRemove);

  void removeByType(const string_t& inputType);
  ::std::function<void()> _addCheckInputs(const ::std::function<void()>& fn);
  void attachInput(ICameraInput<TCamera>* input);
  void attachElement(ICanvas* canvas, bool noPreventDefault = false);
  void detachElement(ICanvas* canvas, bool disconnect = false);
  void rebuildInputCheck();
  void checkInputs();
  unordered_map_t<string_t, shared_ptr_t<ICameraInput<TCamera>>>& attached();

  /**
   * @brief Remove all attached input methods from a camera
   */
  void clear();

  Json::object serialize(Json::object& serializedCamera) const;
  void parse(const Json::value& parsedCamera);

public:
  ICanvas* attachedElement;
  bool noPreventDefault;
  TCamera* camera;

private:
  unordered_map_t<string_t, shared_ptr_t<ICameraInput<TCamera>>> _attached;

}; // end of class CameraInputsManager<TCamera>

} // end of namespace BABYLON

#endif // end of BABYLON_CAMERAS_CAMERA_INPUTS_MANAGER_H
