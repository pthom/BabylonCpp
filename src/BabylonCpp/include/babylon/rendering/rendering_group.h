#ifndef BABYLON_RENDERING_RENDERING_GROUP_H
#define BABYLON_RENDERING_RENDERING_GROUP_H

#include <babylon/babylon_global.h>

namespace BABYLON {

class BABYLON_SHARED_EXPORT RenderingGroup {

public:
  /**
   * Creates a new rendering group.
   * @param index The rendering group index
   * @param opaqueSortCompareFn The opaque sort comparison function. If null no
   * order is applied
   * @param alphaTestSortCompareFn The alpha test sort comparison function. If
   * null no order is applied
   * @param transparentSortCompareFn The transparent sort comparison function.
   * If null back to front + alpha index sort is applied
   */
  RenderingGroup(
    unsigned int index, Scene* scene,
    const ::std::function<int(SubMesh* a, SubMesh* b)>& opaqueSortCompareFn
    = nullptr,
    const ::std::function<int(SubMesh* a, SubMesh* b)>& alphaTestSortCompareFn
    = nullptr,
    const ::std::function<int(SubMesh* a, SubMesh* b)>& transparentSortCompareFn
    = nullptr);
  ~RenderingGroup();

  /**
   * Set the opaque sort comparison function.
   * If null the sub meshes will be render in the order they were created
   */
  void setOpaqueSortCompareFn(
    const ::std::function<int(SubMesh* a, SubMesh* b)>& value);

  /**
   * Set the alpha test sort comparison function.
   * If null the sub meshes will be render in the order they were created
   */
  void setAlphaTestSortCompareFn(
    const ::std::function<int(SubMesh* a, SubMesh* b)>& value);

  /**
   * Set the transparent sort comparison function.
   * If null the sub meshes will be render in the order they were created
   */
  void setTransparentSortCompareFn(
    const ::std::function<int(SubMesh* a, SubMesh* b)>& value);

  /**
   * Render all the sub meshes contained in the group.
   * @param customRenderFunction Used to override the default render behaviour
   * of the group.
   */
  void render(
    ::std::function<void(const vector_t<SubMesh*>& opaqueSubMeshes,
                         const vector_t<SubMesh*>& alphaTestSubMeshes,
                         const vector_t<SubMesh*>& transparentSubMeshes,
                         const vector_t<SubMesh*>& depthOnlySubMeshes,
                         const ::std::function<void()>& beforeTransparents)>&
      customRenderFunction,
    bool renderSprites, bool renderParticles,
    const vector_t<AbstractMesh*> activeMeshes);

  /**
   * Build in function which can be applied to ensure meshes of a special queue
   * (opaque, alpha test, transparent)
   * are rendered back to front if in the same alpha index.
   *
   * @param a The first submesh
   * @param b The second submesh
   * @returns The result of the comparison
   */
  static int defaultTransparentSortCompare(SubMesh* a, SubMesh* b);

  /**
   * Build in function which can be applied to ensure meshes of a special queue
   * (opaque, alpha test, transparent)
   * are rendered back to front.
   *
   * @param a The first submesh
   * @param b The second submesh
   * @returns The result of the comparison
   */
  static int backToFrontSortCompare(SubMesh* a, SubMesh* b);

  /**
   * Build in function which can be applied to ensure meshes of a special queue
   * (opaque, alpha test, transparent)
   * are rendered front to back (prevent overdraw).
   *
   * @param a The first submesh
   * @param b The second submesh
   * @returns The result of the comparison
   */
  static int frontToBackSortCompare(SubMesh* a, SubMesh* b);

  /**
   * Resets the different lists of submeshes to prepare a new frame.
   */
  void prepare();

  void dispose();

  /**
   * Inserts the submesh in its correct queue depending on its material.
   * @param subMesh The submesh to dispatch
   */
  void dispatch(SubMesh* subMesh);

  void dispatchSprites(SpriteManager* spriteManager);

  void dispatchParticles(IParticleSystem* particleSystem);

private:
  /**
   * Renders the opaque submeshes in the order from the opaqueSortCompareFn.
   * @param subMeshes The submeshes to render
   */
  void renderOpaqueSorted(const vector_t<SubMesh*>& subMeshes);

  /**
   * Renders the opaque submeshes in the order from the alphatestSortCompareFn.
   * @param subMeshes The submeshes to render
   */
  void renderAlphaTestSorted(const vector_t<SubMesh*>& subMeshes);

  /**
   * Renders the opaque submeshes in the order from the
   * transparentSortCompareFn.
   * @param subMeshes The submeshes to render
   */
  void renderTransparentSorted(const vector_t<SubMesh*>& subMeshes);

  void _renderParticles(const vector_t<AbstractMesh*>& activeMeshes);

  void _renderSprites();

  /**
   * Renders the submeshes in a specified order.
   * @param subMeshes The submeshes to sort before render
   * @param sortCompareFn The comparison function use to sort
   * @param camera The camera to use to preprocess the submeshes to help sorting
   * @param transparent Specifies to activate blending if true
   */
  static void renderSorted(
    const vector_t<SubMesh*>& subMeshes,
    const ::std::function<int(SubMesh* a, SubMesh* b)>& sortCompareFn,
    Camera* camera, bool transparent);

  /**
   * Renders the submeshes in the order they were dispatched (no sort applied).
   * @param subMeshes The submeshes to render
   */
  static void renderUnsorted(const vector_t<SubMesh*>& subMeshes);

public:
  unsigned int index;
  ::std::function<void()> onBeforeTransparentRendering;

private:
  Scene* _scene;
  vector_t<SubMesh*> _opaqueSubMeshes;
  vector_t<SubMesh*> _transparentSubMeshes;
  vector_t<SubMesh*> _alphaTestSubMeshes;
  vector_t<SubMesh*> _depthOnlySubMeshes;
  vector_t<IParticleSystem*> _particleSystems;
  vector_t<SpriteManager*> _spriteManagers;

  ::std::function<int(SubMesh* a, SubMesh* b)> _opaqueSortCompareFn;
  ::std::function<int(SubMesh* a, SubMesh* b)> _alphaTestSortCompareFn;
  ::std::function<int(SubMesh* a, SubMesh* b)> _transparentSortCompareFn;

  ::std::function<void(const vector_t<SubMesh*>& subMeshes)> _renderOpaque;
  ::std::function<void(const vector_t<SubMesh*>& subMeshes)> _renderAlphaTest;
  ::std::function<void(const vector_t<SubMesh*>& subMeshes)> _renderTransparent;

  vector_t<EdgesRenderer*> _edgesRenderers;

}; // end of class RenderingGroup

} // end of namespace BABYLON

#endif // end of BABYLON_RENDERING_RENDERING_GROUP_H
