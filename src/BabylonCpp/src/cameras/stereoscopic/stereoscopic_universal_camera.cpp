#include <babylon/cameras/stereoscopic/stereoscopic_universal_camera.h>

#include <babylon/core/json_util.h>

namespace BABYLON {

bool StereoscopicUniversalCamera::NodeConstructorAdded = false;

void StereoscopicUniversalCamera::AddNodeConstructor()
{
  Node::AddNodeConstructor(
    "StereoscopicUniversalCamera", [](const std::string& iName, Scene* scene,
                                      const std::optional<json>& options) {
      float interaxialDistance      = 0.f;
      bool isStereoscopicSideBySide = false;
      if (options) {
        interaxialDistance
          = json_util::get_number<float>(*options, "interaxial_distance");
        isStereoscopicSideBySide
          = json_util::get_bool(*options, "isStereoscopicSideBySide");
      }
      return StereoscopicUniversalCamera::New(iName, Vector3::Zero(),
                                              interaxialDistance,
                                              isStereoscopicSideBySide, scene);
    });
  StereoscopicUniversalCamera::NodeConstructorAdded = true;
}

StereoscopicUniversalCamera::StereoscopicUniversalCamera(
  const std::string& iName, const Vector3& iPosition, float iInteraxialDistance,
  bool iIsStereoscopicSideBySide, Scene* scene)
    : UniversalCamera{iName, iPosition, scene}
{
  interaxialDistance       = iInteraxialDistance;
  isStereoscopicSideBySide = iIsStereoscopicSideBySide;
  RigParamaters rigParams;
  rigParams.interaxialDistance = interaxialDistance;
  setCameraRigMode(isStereoscopicSideBySide ?
                     Camera::RIG_MODE_STEREOSCOPIC_SIDEBYSIDE_PARALLEL :
                     Camera::RIG_MODE_STEREOSCOPIC_OVERUNDER,
                   rigParams);
}

StereoscopicUniversalCamera::~StereoscopicUniversalCamera() = default;

const std::string StereoscopicUniversalCamera::getClassName() const
{
  return "StereoscopicUniversalCamera";
}

} // end of namespace BABYLON
