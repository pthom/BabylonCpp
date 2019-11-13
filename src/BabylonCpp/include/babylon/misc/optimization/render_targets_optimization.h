#ifndef BABYLON_MISC_OPTIMIZATION_RENDER_TARGETS_OPTIMIZATION_H
#define BABYLON_MISC_OPTIMIZATION_RENDER_TARGETS_OPTIMIZATION_H

#include <babylon/babylon_api.h>
#include <babylon/misc/optimization/scene_optimization.h>

namespace BABYLON {

class BABYLON_SHARED_EXPORT RenderTargetsOptimization
    : public SceneOptimization {

public:
  RenderTargetsOptimization(int priority = 0);
  ~RenderTargetsOptimization(); // = default

  bool apply(Scene* scene) override;

}; // end of class RenderTargetsOptimization

} // end of namespace BABYLON

#endif // end of BABYLON_MISC_OPTIMIZATION_RENDER_TARGETS_OPTIMIZATION_H
