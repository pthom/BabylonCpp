#include <babylon/gamepads/controllers/pose_enabled_controller.h>

#include <babylon/culling/ray.h>
#include <babylon/engines/scene.h>
#include <babylon/interfaces/ibrowser_gamepad.h>
#include <babylon/math/tmp_vectors.h>
#include <babylon/meshes/abstract_mesh.h>

namespace BABYLON {

PoseEnabledController::PoseEnabledController(const IBrowserGamepadPtr& iBrowserGamepad)
    : Gamepad(iBrowserGamepad->id, iBrowserGamepad->index, iBrowserGamepad)
    , isXR{false}
    , _mesh{nullptr}
    , _deviceToWorld{Matrix::Identity()}
    , _pointingPoseNode{nullptr}
    , mesh{this, &PoseEnabledController::get_mesh}
    , _deviceRoomPosition{Vector3::Zero()}
    , _poseControlledCamera{nullptr}
    , _workingMatrix{Matrix::Identity()}
{
  type              = Gamepad::POSE_ENABLED;
  controllerType    = PoseEnabledControllerType::GENERIC;
  devicePosition    = Vector3::Zero();
  deviceScaleFactor = 1.f;
  position          = Vector3::Zero();

  // Used to convert 6dof controllers to 3dof
  _trackPosition              = true;
  _maxRotationDistFromHeadset = Math::PI / 5.f;
  _draggedRoomRotation        = 0.f;

  _calculatedPosition = Vector3::Zero();
  Quaternion::RotationYawPitchRollToRef(Math::PI, 0, 0, _leftHandSystemQuaternion);
}

PoseEnabledController::~PoseEnabledController() = default;

void PoseEnabledController::_disableTrackPosition(const Vector3& fixedPosition)
{
  if (_trackPosition) {
    _calculatedPosition.copyFrom(fixedPosition);
    _trackPosition = false;
  }
}

void PoseEnabledController::update()
{
  if (isXR) {
    return;
  }
  Gamepad::update();
  _updatePoseAndMesh();
}

void PoseEnabledController::_updatePoseAndMesh()
{
  if (isXR) {
    return;
  }
  const auto& pose = browserGamepad->pose;
  if (pose) {
    updateFromDevice(*pose);
  }

  Vector3::TransformCoordinatesToRef(_calculatedPosition, _deviceToWorld, devicePosition);
  _deviceToWorld.getRotationMatrixToRef(_workingMatrix);
  Quaternion::FromRotationMatrixToRef(_workingMatrix, deviceRotationQuaternion);
  deviceRotationQuaternion.multiplyInPlace(_calculatedRotation);

  if (_mesh) {
    _mesh->position().copyFrom(devicePosition);
    if (_mesh->rotationQuaternion()) {
      _mesh->rotationQuaternion()->copyFrom(deviceRotationQuaternion);
    }
  }
}

void PoseEnabledController::updateFromDevice(const DevicePose& poseData)
{
  if (isXR) {
    return;
  }
  rawPose = poseData;
  if (!poseData.position.empty()) {
    _deviceRoomPosition.copyFromFloats(poseData.position[0], poseData.position[1],
                                       -poseData.position[2]);
    if (_mesh && _mesh->getScene()->useRightHandedSystem()) {
      _deviceRoomPosition.z *= -1.f;
    }

    _deviceRoomPosition.scaleToRef(deviceScaleFactor, _calculatedPosition);
    _calculatedPosition.addInPlace(position);
  }
  auto& pose = rawPose;
  if (!poseData.orientation.empty() && !pose.orientation.empty() && pose.orientation.size() == 4) {
    _deviceRoomRotationQuaternion.copyFromFloats(pose.orientation[0], pose.orientation[1],
                                                 -pose.orientation[2], -pose.orientation[3]);
    if (_mesh) {
      if (_mesh->getScene()->useRightHandedSystem()) {
        _deviceRoomRotationQuaternion.z *= -1.f;
        _deviceRoomRotationQuaternion.w *= -1.f;
      }
      else {
        _deviceRoomRotationQuaternion.multiplyToRef(_leftHandSystemQuaternion,
                                                    deviceRotationQuaternion);
      }
    }

    // if the camera is set, rotate to the camera's rotation
    _deviceRoomRotationQuaternion.multiplyToRef(rotationQuaternion, _calculatedRotation);
  }
}

void PoseEnabledController::attachToMesh(const AbstractMeshPtr& iMesh)
{
  if (_mesh) {
    _mesh->setParent(nullptr);
  }
  _mesh = iMesh;
  if (_poseControlledCamera) {
    // _mesh->setParent(_poseControlledCamera);
  }
  if (!_mesh->rotationQuaternion()) {
    _mesh->rotationQuaternion = Quaternion();
  }

  // Sync controller mesh and pointing pose node's state with controller, this
  // is done to avoid a frame where position is 0,0,0 when attaching mesh
  if (!isXR) {
    _updatePoseAndMesh();
    if (_pointingPoseNode) {
      std::vector<Node*> parents;
      auto obj = static_cast<Node*>(_pointingPoseNode);
      while (obj && obj->parent()) {
        parents.emplace_back(obj->parent());
        obj = obj->parent();
      }
      std::reverse(parents.begin(), parents.end());
      for (auto& p : parents) {
        p->computeWorldMatrix(true);
      }
    }
  }

  _meshAttachedObservable.notifyObservers(iMesh.get());
}

void PoseEnabledController::attachToPoseControlledCamera(TargetCamera* camera)
{
  _poseControlledCamera = camera;
  if (_mesh) {
    //  _mesh->parent = _poseControlledCamera;
  }
}

void PoseEnabledController::dispose()
{
  if (_mesh) {
    _mesh->dispose();
  }
  _mesh = nullptr;

  Gamepad::dispose();
}

AbstractMeshPtr& PoseEnabledController::get_mesh()
{
  return _mesh;
}

Ray PoseEnabledController::getForwardRay(float length)
{
  if (!mesh()) {
    return Ray(Vector3::Zero(), Vector3{0.f, 0.f, 1.f}, length);
  }

  auto m      = _pointingPoseNode ? _pointingPoseNode->getWorldMatrix() : mesh()->getWorldMatrix();
  auto origin = m.getTranslation();

  Vector3 forward{0.f, 0.f, -1.f};
  auto forwardWorld = Vector3::TransformNormal(forward, m);

  auto direction = Vector3::Normalize(forwardWorld);

  return Ray(origin, direction, length);
}

} // end of namespace BABYLON
