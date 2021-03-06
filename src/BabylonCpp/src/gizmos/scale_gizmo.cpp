#include <babylon/gizmos/scale_gizmo.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/behaviors/meshes/pointer_drag_behavior.h>
#include <babylon/core/logging.h>
#include <babylon/gizmos/axis_scale_gizmo.h>
#include <babylon/lights/hemispheric_light.h>
#include <babylon/math/color3.h>
#include <babylon/math/vector3.h>
#include <babylon/meshes/builders/mesh_builder_options.h>
#include <babylon/meshes/builders/polyhedron_builder.h>
#include <babylon/meshes/mesh.h>

namespace BABYLON {

ScaleGizmo::ScaleGizmo(const std::shared_ptr<UtilityLayerRenderer>& iGizmoLayer)
    : Gizmo{iGizmoLayer}
    , snapDistance{this, &ScaleGizmo::get_snapDistance,
                   &ScaleGizmo::set_snapDistance}
{
  xGizmo = std::make_unique<AxisScaleGizmo>(
    Vector3(1.f, 0.f, 0.f), Color3::Red().scale(0.5f), iGizmoLayer);
  yGizmo = std::make_unique<AxisScaleGizmo>(
    Vector3(0.f, 1.f, 0.f), Color3::Green().scale(0.5f), iGizmoLayer);
  zGizmo = std::make_unique<AxisScaleGizmo>(
    Vector3(0.f, 0.f, 1.f), Color3::Blue().scale(0.5f), iGizmoLayer);

  // Create uniform scale gizmo
  uniformScaleGizmo = std::make_unique<AxisScaleGizmo>(
    Vector3(0.f, 1.f, 0.f), Color3::Yellow().scale(0.5f), iGizmoLayer);
  uniformScaleGizmo->updateGizmoRotationToMatchAttachedMesh = false;
  uniformScaleGizmo->uniformScaling                         = true;
  PolyhedronOptions options;
  options.type            = 1;
  auto uniformScalingMesh = PolyhedronBuilder::CreatePolyhedron(
    "", options, uniformScaleGizmo->gizmoLayer->utilityLayerScene.get());
  uniformScalingMesh->scaling().scaleInPlace(0.02f);
  uniformScalingMesh->visibility = 0.f;
  auto octahedron                = PolyhedronBuilder::CreatePolyhedron(
    "", options, uniformScaleGizmo->gizmoLayer->utilityLayerScene.get());
  octahedron->scaling().scaleInPlace(0.007f);
  uniformScalingMesh->addChild(*octahedron);
  uniformScaleGizmo->setCustomMesh(uniformScalingMesh, true);
  const auto& light = gizmoLayer->_getSharedGizmoLight();
  light->includedOnlyMeshes
    = stl_util::concat(light->includedOnlyMeshes(), {octahedron});

  // Relay drag events
  for (const auto& gizmo :
       {xGizmo.get(), yGizmo.get(), zGizmo.get(), uniformScaleGizmo.get()}) {
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

ScaleGizmo::~ScaleGizmo()
{
}

AbstractMeshPtr& ScaleGizmo::get_attachedMesh()
{
  return xGizmo->attachedMesh;
}

void ScaleGizmo::set_attachedMesh(const AbstractMeshPtr& mesh)
{
  if (xGizmo) {
    xGizmo->attachedMesh            = mesh;
    yGizmo->attachedMesh            = mesh;
    zGizmo->attachedMesh            = mesh;
    uniformScaleGizmo->attachedMesh = mesh;
  }
}

void ScaleGizmo::set_updateGizmoRotationToMatchAttachedMesh(bool value)
{
  if (!value) {
    BABYLON_LOG_WARN("ScaleGizmo",
                     "Setting updateGizmoRotationToMatchAttachedMesh = false "
                     "on scaling gizmo is not supported.")
  }
  if (xGizmo) {
    xGizmo->updateGizmoRotationToMatchAttachedMesh = value;
    yGizmo->updateGizmoRotationToMatchAttachedMesh = value;
    zGizmo->updateGizmoRotationToMatchAttachedMesh = value;
  }
}

bool ScaleGizmo::get_updateGizmoRotationToMatchAttachedMesh() const
{
  return xGizmo->updateGizmoRotationToMatchAttachedMesh;
}

void ScaleGizmo::set_snapDistance(float value)
{
  if (xGizmo) {
    xGizmo->snapDistance            = value;
    yGizmo->snapDistance            = value;
    zGizmo->snapDistance            = value;
    uniformScaleGizmo->snapDistance = value;
  }
}

float ScaleGizmo::get_snapDistance() const
{
  return xGizmo->snapDistance;
}

void ScaleGizmo::set_scaleRatio(float value)
{
  if (xGizmo) {
    xGizmo->scaleRatio            = value;
    yGizmo->scaleRatio            = value;
    zGizmo->scaleRatio            = value;
    uniformScaleGizmo->scaleRatio = value;
  }
}

float ScaleGizmo::get_scaleRatio() const
{
  return xGizmo->scaleRatio;
}

void ScaleGizmo::dispose(bool doNotRecurse, bool disposeMaterialAndTextures)
{
  xGizmo->dispose(doNotRecurse, disposeMaterialAndTextures);
  yGizmo->dispose(doNotRecurse, disposeMaterialAndTextures);
  zGizmo->dispose(doNotRecurse, disposeMaterialAndTextures);
  uniformScaleGizmo->dispose(doNotRecurse, disposeMaterialAndTextures);
  onDragStartObservable.clear();
  onDragEndObservable.clear();
}

} // end of namespace BABYLON
