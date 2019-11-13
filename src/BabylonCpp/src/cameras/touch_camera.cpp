#include <babylon/cameras/touch_camera.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/cameras/inputs/free_camera_mouse_input.h>
#include <babylon/cameras/inputs/free_camera_touch_input.h>

namespace BABYLON {

bool TouchCamera::NodeConstructorAdded = false;

void TouchCamera::AddNodeConstructor()
{
  Node::AddNodeConstructor(
    "TouchCamera", [](const std::string& iName, Scene* scene,
                      const std::optional<json>& /*options*/) {
      return TouchCamera::New(iName, Vector3::Zero(), scene);
    });
  TouchCamera::NodeConstructorAdded = true;
}

//-- Begin properties for backward compatibility for inputs

float TouchCamera::get_touchAngularSensibility() const
{
  auto& inputsAttached = inputs->attached;
  if (stl_util::contains(inputsAttached, "touch") && inputsAttached["touch"]) {
    auto touch
      = static_cast<FreeCameraTouchInput*>(inputsAttached["touch"].get());
    if (touch) {
      return touch->touchAngularSensibility;
    }
  }

  return 0.f;
}

void TouchCamera::set_touchAngularSensibility(float value)
{
  auto& inputsAttached = inputs->attached;
  if (stl_util::contains(inputsAttached, "touch") && inputsAttached["touch"]) {
    auto touch
      = static_cast<FreeCameraTouchInput*>(inputsAttached["touch"].get());
    if (touch) {
      touch->touchAngularSensibility = value;
    }
  }
}

float TouchCamera::get_touchMoveSensibility() const
{
  auto& inputsAttached = inputs->attached;
  if (stl_util::contains(inputsAttached, "touch") && inputsAttached["touch"]) {
    auto touch
      = static_cast<FreeCameraTouchInput*>(inputsAttached["touch"].get());
    if (touch) {
      return touch->touchMoveSensibility;
    }
  }

  return 0.f;
}

void TouchCamera::set_touchMoveSensibility(float value)
{
  auto& inputsAttached = inputs->attached;
  if (stl_util::contains(inputsAttached, "touch") && inputsAttached["touch"]) {
    auto touch
      = static_cast<FreeCameraTouchInput*>(inputsAttached["touch"].get());
    if (touch) {
      touch->touchMoveSensibility = value;
    }
  }
}

//-- end properties for backward compatibility for inputs

TouchCamera::TouchCamera::TouchCamera(const std::string& iName,
                                      const Vector3& iPosition, Scene* scene)
    : FreeCamera{iName, iPosition, scene}
    , touchAngularSensibility{this, &TouchCamera::get_touchAngularSensibility,
                              &TouchCamera::set_touchAngularSensibility}
    , touchMoveSensibility{this, &TouchCamera::get_touchMoveSensibility,
                           &TouchCamera::set_touchMoveSensibility}
{
  inputs->addTouch();

  _setupInputs();
}

TouchCamera::~TouchCamera() = default;

const std::string TouchCamera::getClassName() const
{
  return "TouchCamera";
}

void TouchCamera::_setupInputs()
{
  auto& inputsAttached = inputs->attached;
  if (stl_util::contains(inputsAttached, "mouse") && inputsAttached["mouse"]) {
    auto mouse
      = static_cast<FreeCameraMouseInput*>(inputsAttached["mouse"].get());
    if (mouse) {
      mouse->touchEnabled = false;
    }
  }
}

} // end of namespace BABYLON
