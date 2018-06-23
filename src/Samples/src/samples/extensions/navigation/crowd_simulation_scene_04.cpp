#include <babylon/samples/extensions/navigation/crowd_simulation_scene_04.h>

#include <babylon/cameras/arc_rotate_camera.h>
#include <babylon/core/logging.h>
#include <babylon/engine/engine.h>
#include <babylon/engine/scene.h>
#include <babylon/extensions/navigation/crowd_simulation.h>
#include <babylon/interfaces/icanvas.h>
#include <babylon/lights/hemispheric_light.h>
#include <babylon/materials/standard_material.h>
#include <babylon/mesh/mesh.h>
#include <babylon/samples/extensions/navigation/crowd_simulation_utils.h>

namespace BABYLON {
namespace Samples {

CrowdSimulationScene04::CrowdSimulationScene04(ICanvas* iCanvas)
    : IRenderableScene(iCanvas), _crowdSimulation{nullptr}
{
}

CrowdSimulationScene04::~CrowdSimulationScene04()
{
}

const char* CrowdSimulationScene04::getName()
{
  return "Crowd Simulation Scene 04";
}

void CrowdSimulationScene04::initializeScene(ICanvas* canvas, Scene* scene)
{
  // Change the clear color
  scene->clearColor = Color4(0.36f, 0.36f, 0.36f);

  // Create a camera
  auto camera
    = ArcRotateCamera::New("camera", 0.f, 0.f, 10.f, Vector3::Zero(), scene);

  // Set the camera position and limits
  camera->setPosition(Vector3(0.f, 500.f, 0.1f));
  camera->lowerBetaLimit = 0.1f;
  camera->upperBetaLimit = Math::PI_2 * 0.95f;

  // Attach the camera to the canvas
  camera->attachControl(canvas, true);

  // Create a basic light, aiming 0,1,0 - meaning, to the sky
  auto light       = HemisphericLight::New("light1", Vector3(0, 1, 0), scene);
  light->intensity = 1.f;

  // Create the ground material
  auto groundMaterial = StandardMaterial::New("groundMaterial", scene);
  groundMaterial->setEmissiveColor(Color3::Black());
  groundMaterial->setSpecularColor(Color3::Black());
  groundMaterial->setDiffuseColor(Color3(0.36f, 0.36f, 0.36f));

  // Create the ground
  auto ground            = Mesh::CreateBox("ground", 600.f, scene);
  ground->position().y   = -6.f;
  ground->scaling().y    = 0.001f;
  ground->material       = groundMaterial;
  ground->receiveShadows = true;

  // Setup Scenario
  setupScenario(scene);

  // Update scene model
  scene->onBeforeCameraRenderObservable.add(
    [this](Camera*, EventState&) { _crowdSimulation->update(); });
}

void CrowdSimulationScene04::setupScenario(Scene* scene)
{
  // Create agent meshes
  _agentMeshes = CrowdSimulationUtils::createAgentMeshes(scene, 128);

  // Create new crowd simulation
  _crowdSimulation.reset(new Extensions::CrowdSimulation());
  _crowdSimulation->setTimeStep(0.3f);

  // Create agents
  for (size_t agentNo = 0; agentNo < _agentMeshes.size(); ++agentNo) {
    auto agentMesh = _agentMeshes[agentNo];
    auto agentId   = _crowdSimulation->addAgent(agentMesh);
    _crowdSimulation->setAgentGoal(
      agentId, Vector2(-agentMesh->position().x, -agentMesh->position().z));
    _crowdSimulation->setAgentMaxSpeed(agentId, 1.f);
  }

  // Create obstacle
  auto box = Mesh::CreateBox("", 60.f, scene);
  _crowdSimulation->addObstacleByBoundingBox(box, Vector3(0.f, -3.f, 0.f));

  // Add waypoints
  const vector_t<Vector2> wayPoints{
    Vector2(33.f, 33.f),   //
    Vector2(-33.f, 33.f),  //
    Vector2(-33.f, -33.f), //
    Vector2(33.f, -33.f)   //
  };

  for (const auto& wayPoint : wayPoints) {
    CrowdSimulationUtils::createWayPointMesh(scene, wayPoint);
    _crowdSimulation->addWayPoint(wayPoint);
  }

  // Intialize the simulation
  _crowdSimulation->processObstacles();
  _crowdSimulation->computeRoadMap();
}

} // end of namespace Samples
} // end of namespace BABYLON
