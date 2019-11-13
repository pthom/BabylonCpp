#include <babylon/bones/bone_ik_controller.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/bones/bone.h>
#include <babylon/meshes/abstract_mesh.h>

namespace BABYLON {

std::array<Vector3, 6> BoneIKController::_tmpVecs{
  {Vector3::Zero(), Vector3::Zero(), Vector3::Zero(), Vector3::Zero(),
   Vector3::Zero(), Vector3::Zero()}};
Quaternion BoneIKController::_tmpQuat{Quaternion::Identity()};
std::array<Matrix, 2> BoneIKController::_tmpMats{
  {Matrix::Identity(), Matrix::Identity()}};

BoneIKController::BoneIKController(
  AbstractMesh* iMesh, Bone* bone,
  const std::optional<BoneIKControllerOptions>& iOptions)
    : targetMesh{nullptr}
    , poleTargetMesh{nullptr}
    , targetPosition{Vector3::Zero()}
    , poleTargetPosition{Vector3::Zero()}
    , poleTargetLocalOffset{Vector3::Zero()}
    , poleAngle{0.f}
    , slerpAmount{1.f}
    , maxAngle{this, &BoneIKController::get_maxAngle,
               &BoneIKController::set_maxAngle}
    , _bone1Quat{Quaternion::Identity()}
    , _bone1Mat{Matrix::Identity()}
    , _bone2Ang{Math::PI}
    , _maxAngle{Math::PI}
    , _rightHandedSystem{false}
    , _bendAxis{Vector3::Right()}
    , _slerping{false}
    , _adjustRoll{0.f}
{
  _bone2 = bone;
  _bone1 = bone->getParent();

  if (!_bone1) {
    return;
  }

  mesh = iMesh;

  auto bonePos = bone->getPosition();

  if (bone->getAbsoluteTransform().determinant() > 0.f) {
    _rightHandedSystem = true;
    _bendAxis.x        = 0.f;
    _bendAxis.y        = 0.f;
    _bendAxis.z        = -1.f;

    if (bonePos.x > bonePos.y && bonePos.x > bonePos.z) {
      _adjustRoll = Math::PI * 0.5f;
      _bendAxis.z = 1;
    }
  }

  if (_bone1->length > 0.f) {

    auto boneScale1 = *_bone1->getScale();
    auto boneScale2 = *_bone2->getScale();

    _bone1Length = _bone1->length * boneScale1.y * mesh->scaling().y;
    _bone2Length = _bone2->length * boneScale2.y * mesh->scaling().y;
  }
  else if (!_bone1->children.empty() && _bone1->children[0]) {

    mesh->computeWorldMatrix(true);

    auto pos1 = _bone2->children[0]->getAbsolutePosition(mesh);
    auto pos2 = _bone2->getAbsolutePosition(mesh);
    auto pos3 = _bone1->getAbsolutePosition(mesh);

    _bone1Length = Vector3::Distance(pos1, pos2);
    _bone2Length = Vector3::Distance(pos2, pos3);
  }

  _bone1->getRotationMatrixToRef(_bone1Mat, Space::WORLD, mesh);
  maxAngle = Math::PI;

  // Options
  if (iOptions) {
    const auto& options = *iOptions;
    if (options.targetMesh) {
      targetMesh = options.targetMesh;
      targetMesh->computeWorldMatrix(true);
    }

    if (options.poleTargetMesh) {

      poleTargetMesh = options.poleTargetMesh;
      poleTargetMesh->computeWorldMatrix(true);
    }
    else if (options.poleTargetBone) {
      poleTargetBone = options.poleTargetBone;
    }
    else if (_bone1->getParent()) {
      poleTargetBone = _bone1->getParent();
    }

    if (options.poleTargetLocalOffset) {
      poleTargetLocalOffset.copyFrom(*options.poleTargetLocalOffset);
    }

    if (options.poleAngle) {
      poleAngle = *options.poleAngle;
    }

    if (options.bendAxis) {
      _bendAxis.copyFrom(*options.bendAxis);
    }

    if (options.maxAngle) {
      maxAngle = *options.maxAngle;
    }

    if (options.slerpAmount) {
      slerpAmount = *options.slerpAmount;
    }
  }
}

BoneIKController::~BoneIKController() = default;

float BoneIKController::get_maxAngle() const
{
  return _maxAngle;
}

void BoneIKController::set_maxAngle(float value)
{
  _setMaxAngle(value);
}

void BoneIKController::_setMaxAngle(float ang)
{
  if (ang < 0.f) {
    ang = 0.f;
  }

  if (ang > Math::PI) {
    ang = Math::PI;
  }

  _maxAngle = ang;

  const float a = _bone1Length;
  const float b = _bone2Length;

  _maxReach = std::sqrt(a * a + b * b - 2 * a * b * std::cos(ang));
}

void BoneIKController::update()
{
  auto& bone1 = _bone1;

  if (!bone1) {
    return;
  }

  auto& target     = targetPosition;
  auto& poleTarget = poleTargetPosition;

  auto& mat1 = BoneIKController::_tmpMats[0];
  auto& mat2 = BoneIKController::_tmpMats[1];

  if (targetMesh) {
    target.copyFrom(targetMesh->getAbsolutePosition());
  }

  if (poleTargetBone) {
    poleTargetBone->getAbsolutePositionFromLocalToRef(poleTargetLocalOffset,
                                                      mesh, poleTarget);
  }
  else if (poleTargetMesh) {
    Vector3::TransformCoordinatesToRef(
      poleTargetLocalOffset, poleTargetMesh->getWorldMatrix(), poleTarget);
  }

  auto& bonePos = BoneIKController::_tmpVecs[0];
  auto& zaxis   = BoneIKController::_tmpVecs[1];
  auto& xaxis   = BoneIKController::_tmpVecs[2];
  auto& yaxis   = BoneIKController::_tmpVecs[3];
  auto& upAxis  = BoneIKController::_tmpVecs[4];

  auto& _iTmpQuat = BoneIKController::_tmpQuat;

  bone1->getAbsolutePositionToRef(mesh, bonePos);

  poleTarget.subtractToRef(bonePos, upAxis);

  if (stl_util::almost_equal(upAxis.x, 0.f)
      && stl_util::almost_equal(upAxis.y, 0.f)
      && stl_util::almost_equal(upAxis.z, 0.f)) {
    upAxis.y = 1.f;
  }
  else {
    upAxis.normalize();
  }

  target.subtractToRef(bonePos, yaxis);
  yaxis.normalize();

  Vector3::CrossToRef(yaxis, upAxis, zaxis);
  zaxis.normalize();

  Vector3::CrossToRef(yaxis, zaxis, xaxis);
  xaxis.normalize();

  Matrix::FromXYZAxesToRef(xaxis, yaxis, zaxis, mat1);

  const float a = _bone1Length;
  const float b = _bone2Length;

  float c = Vector3::Distance(bonePos, target);

  if (_maxReach > 0.f) {
    c = std::min(_maxReach, c);
  }

  float acosa = (b * b + c * c - a * a) / (2.f * b * c);
  float acosb = (c * c + a * a - b * b) / (2.f * c * a);

  if (acosa > 1.f) {
    acosa = 1.f;
  }

  if (acosb > 1.f) {
    acosb = 1.f;
  }

  if (acosa < -1.f) {
    acosa = -1.f;
  }

  if (acosb < -1.f) {
    acosb = -1.f;
  }

  const float angA = std::acos(acosa);
  const float angB = std::acos(acosb);

  float angC = -angA - angB;

  if (_rightHandedSystem) {
    Matrix::RotationYawPitchRollToRef(0.f, 0.f, _adjustRoll, mat2);
    mat2.multiplyToRef(mat1, mat1);

    Matrix::RotationAxisToRef(_bendAxis, angB, mat2);
    mat1.multiplyToRef(mat2, mat1);
  }
  else {
    auto& _tmpVec = BoneIKController::_tmpVecs[5];

    _tmpVec.copyFrom(_bendAxis);
    _tmpVec.x *= -1.f;

    Matrix::RotationAxisToRef(_tmpVec, -angB, mat2);
    mat2.multiplyToRef(mat1, mat1);
  }

  if (poleAngle != 0.f) {
    Matrix::RotationAxisToRef(yaxis, poleAngle, mat2);
    mat1.multiplyToRef(mat2, mat1);
  }

  if (_bone1) {
    if (slerpAmount < 1.f) {
      if (!_slerping) {
        Quaternion::FromRotationMatrixToRef(_bone1Mat, _bone1Quat);
      }
      Quaternion::FromRotationMatrixToRef(mat1, _iTmpQuat);
      Quaternion::SlerpToRef(_bone1Quat, _iTmpQuat, slerpAmount, _bone1Quat);
      angC = _bone2Ang * (1.f - slerpAmount) + angC * slerpAmount;
      _bone1->setRotationQuaternion(_bone1Quat, Space::WORLD, mesh);
      _slerping = true;
    }
    else {
      _bone1->setRotationMatrix(mat1, Space::WORLD, mesh);
      _bone1Mat.copyFrom(mat1);
      _slerping = false;
    }
  }

  _bone2->setAxisAngle(_bendAxis, angC, Space::LOCAL);
  _bone2Ang = angC;
}

} // end of namespace BABYLON
