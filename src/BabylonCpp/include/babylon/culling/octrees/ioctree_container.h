#ifndef BABYLON_CULLING_OCTREES_IOCTREE_CONTAINER_H
#define BABYLON_CULLING_OCTREES_IOCTREE_CONTAINER_H

#include <vector>

#include <babylon/babylon_api.h>

namespace BABYLON {

template <class T>
class OctreeBlock;

/**
 * @brief Contains an array of blocks representing the octree.
 */
template <class T>
struct BABYLON_SHARED_EXPORT IOctreeContainer {
  /**
   * Blocks within the octree
   */
  static std::vector<OctreeBlock<T>> blocks;
}; // end of struct IOctreeContainer<T>

template <class T>
std::vector<OctreeBlock<T>> IOctreeContainer<T>::blocks;

} // end of namespace BABYLON

#endif // end of BABYLON_CULLING_OCTREES_IOCTREE_CONTAINER_H
