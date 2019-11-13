#include <babylon/gizmos/position_gizmo.h>

#include <babylon/behaviors/meshes/pointer_drag_behavior.h>
#include <babylon/core/logging.h>
#include <babylon/gizmos/axis_drag_gizmo.h>
#include <babylon/gizmos/plane_drag_gizmo.h>
#include <babylon/math/color3.h>
#include <babylon/math/vector3.h>

namespace BABYLON {

PositionGizmo::PositionGizmo(const UtilityLayerRendererPtr& iGizmoLayer)
    : Gizmo{iGizmoLayer}
    , planarGizmoEnabled{this, &PositionGizmo::get_planarGizmoEnabled,
                         &PositionGizmo::set_planarGizmoEnabled}
    , snapDistance{this, &PositionGizmo::get_snapDistance,
                   &PositionGizmo::set_snapDistance}
    , _meshAttached{nullptr}
    , _updateGizmoRotationToMatchAttachedMesh{false}
    , _snapDistance{0.f}
    , _scaleRatio{1.f}
    , _planarGizmoEnabled{false}
{
  xGizmo = std::make_unique<AxisDragGizmo>(
    Vector3(1.f, 0.f, 0.f), Color3::Red().scale(0.5f), iGizmoLayer, this);
  yGizmo = std::make_unique<AxisDragGizmo>(
    Vector3(0.f, 1.f, 0.f), Color3::Green().scale(0.5f), iGizmoLayer, this);
  zGizmo = std::make_unique<AxisDragGizmo>(
    Vector3(0.f, 0.f, 1.f), Color3::Blue().scale(0.5f), iGizmoLayer, this);

  xPlaneGizmo = std::make_unique<PlaneDragGizmo>(
    Vector3(1.f, 0.f, 0.f), Color3::Red().scale(0.5f), iGizmoLayer, this);
  yPlaneGizmo = std::make_unique<PlaneDragGizmo>(
    Vector3(0.f, 1.f, 0.f), Color3::Green().scale(0.5f), iGizmoLayer, this);
  zPlaneGizmo = std::make_unique<PlaneDragGizmo>(
    Vector3(0.f, 0.f, 1.f), Color3::Blue().scale(0.5f), iGizmoLayer, this);

  // Relay drag events
  for (const auto& gizmo : {xGizmo.get(), yGizmo.get(), zGizmo.get()}) {
    gizmo->dragBehavior->onDragStartObservable.add(
      [&](DragStartOrEndEvent* /*event*/, EventState& /*es*/) {
        onDragStartObservable.notifyObservers(nullptr);
      });
    gizmo->dragBehavior->onDragEndObservable.add(
      [&](DragStartOrEndEvent* /*event*/, EventState& /*es*/) {
        onDragEndObservable.notifyObservers(nullptr);
      });
  }
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    gizmo->dragBehavior->onDragStartObservable.add(
      [&](DragStartOrEndEvent* /*event*/, EventState& /*es*/) {
        onDragStartObservable.notifyObservers(nullptr);
      });
    gizmo->dragBehavior->onDragEndObservable.add(
      [&](DragStartOrEndEvent* /*event*/, EventState& /*es*/) {
        onDragEndObservable.notifyObservers(nullptr);
      });
  }

  attachedMesh = nullptr;
}

PositionGizmo::~PositionGizmo() = default;

AbstractMeshPtr& PositionGizmo::get_attachedMesh()
{
  return _meshAttached;
}

void PositionGizmo::set_attachedMesh(const AbstractMeshPtr& mesh)
{
  _meshAttached = mesh;
  for (const auto& gizmo : {xGizmo.get(), yGizmo.get(), zGizmo.get()}) {
    if (gizmo->isEnabled()) {
      gizmo->attachedMesh = mesh;
    }
    else {
      gizmo->attachedMesh = nullptr;
    }
  }
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    if (gizmo->isEnabled()) {
      gizmo->attachedMesh = mesh;
    }
    else {
      gizmo->attachedMesh = nullptr;
    }
  }
}

void PositionGizmo::set_planarGizmoEnabled(bool value)
{
  _planarGizmoEnabled = value;
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    if (gizmo) {
      gizmo->isEnabled = value;
      if (value) {
        gizmo->attachedMesh = attachedMesh();
      }
    }
  }
}

bool PositionGizmo::get_planarGizmoEnabled() const
{
  return _planarGizmoEnabled;
}

void PositionGizmo::set_updateGizmoRotationToMatchAttachedMesh(bool value)
{
  _updateGizmoRotationToMatchAttachedMesh = value;
  for (const auto& gizmo : {xGizmo.get(), yGizmo.get(), zGizmo.get()}) {
    if (gizmo) {
      gizmo->updateGizmoRotationToMatchAttachedMesh = value;
    }
  }
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    if (gizmo) {
      gizmo->updateGizmoRotationToMatchAttachedMesh = value;
    }
  }
}

bool PositionGizmo::get_updateGizmoRotationToMatchAttachedMesh() const
{
  return _updateGizmoRotationToMatchAttachedMesh;
}

void PositionGizmo::set_snapDistance(float value)
{
  _snapDistance = value;
  for (const auto& gizmo : {xGizmo.get(), yGizmo.get(), zGizmo.get()}) {
    if (gizmo) {
      gizmo->snapDistance = value;
    }
  }
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    if (gizmo) {
      gizmo->snapDistance = value;
    }
  }
}

float PositionGizmo::get_snapDistance() const
{
  return _snapDistance;
}

void PositionGizmo::set_scaleRatio(float value)
{
  _scaleRatio = value;
  for (const auto& gizmo : {xGizmo.get(), yGizmo.get(), zGizmo.get()}) {
    if (gizmo) {
      gizmo->scaleRatio = value;
    }
  }
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    if (gizmo) {
      gizmo->scaleRatio = value;
    }
  }
}

float PositionGizmo::get_scaleRatio() const
{
  return _scaleRatio;
}

void PositionGizmo::dispose(bool doNotRecurse, bool disposeMaterialAndTextures)
{
  for (const auto& gizmo : {xGizmo.get(), yGizmo.get(), zGizmo.get()}) {
    if (gizmo) {
      gizmo->dispose(doNotRecurse, disposeMaterialAndTextures);
    }
  }
  for (const auto& gizmo :
       {xPlaneGizmo.get(), yPlaneGizmo.get(), zPlaneGizmo.get()}) {
    if (gizmo) {
      gizmo->dispose(doNotRecurse, disposeMaterialAndTextures);
    }
  }
  onDragStartObservable.clear();
  onDragEndObservable.clear();
}

void PositionGizmo::setCustomMesh(const MeshPtr& /*mesh*/,
                                  bool /*useGizmoMaterial*/)
{
  BABYLON_LOG_ERROR(
    "PositionGizmo",
    "Custom meshes are not supported on this gizmo, please set "
    "the custom meshes on the gizmos contained within this one "
    "(gizmo.xGizmo, gizmo.yGizmo, gizmo.zGizmo,gizmo.xPlaneGizmo, "
    "gizmo.yPlaneGizmo, gizmo.zPlaneGizmo)")
}

} // end of namespace BABYLON
