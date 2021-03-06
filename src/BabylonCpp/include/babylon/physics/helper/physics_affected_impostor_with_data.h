#ifndef BABYLON_PHYSICS_HELPER_PHYSICS_AFFECTED_IMPOSTER_WITH_DATA_H
#define BABYLON_PHYSICS_HELPER_PHYSICS_AFFECTED_IMPOSTER_WITH_DATA_H

#include <memory>

#include <babylon/babylon_api.h>

namespace BABYLON {

class PhysicsImpostor;
struct PhysicsHitData;
using PhysicsImpostorPtr = std::shared_ptr<PhysicsImpostor>;
using PhysicsHitDataPtr  = std::shared_ptr<PhysicsHitData>;

/**
 * @brief Interface for an affected physics impostor.
 * @see
 * https://doc.babylonjs.com/how_to/using_the_physics_engine#further-functionality-of-the-impostor-class
 */
struct BABYLON_SHARED_EXPORT PhysicsAffectedImpostorWithData {
  /**
   * The impostor affected by the effect
   */
  PhysicsImpostorPtr impostor = nullptr;

  /**
   * The data about the hit/horce from the explosion
   */
  PhysicsHitDataPtr hitData = nullptr;
}; // end of struct PhysicsAffectedImpostorWithData

} // end of namespace BABYLON

#endif // end of BABYLON_PHYSICS_HELPER_PHYSICS_AFFECTED_IMPOSTER_WITH_DATA_H
