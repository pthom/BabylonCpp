#ifndef BABYLON_INSPECTOR_COMPONENTS_SCENE_EXPLORER_SCENE_GRAPH_COMPONENT_H
#define BABYLON_INSPECTOR_COMPONENTS_SCENE_EXPLORER_SCENE_GRAPH_COMPONENT_H

#include <babylon/babylon_api.h>
#include <babylon/core/string.h>
#include <babylon/core/tree.h>
#include <babylon/imgui/icons_font_awesome_5.h>

#include <babylon/inspector/components/sceneexplorer/tree_item_component.h>

namespace BABYLON {

class BaseTexture;
struct EntityInfo;
class GlobalState;
class Material;
class Node;
class Scene;
class Skeleton;
struct TreeItemComponent;
using BaseTexturePtr = std::shared_ptr<BaseTexture>;
using GlobalStatePtr = std::shared_ptr<GlobalState>;
using MaterialPtr    = std::shared_ptr<Material>;
using NodePtr        = std::shared_ptr<Node>;
using SkeletonPtr    = std::shared_ptr<Skeleton>;
using TreeItem       = TreeItemComponent;

struct SceneGraphComponentProps {
  Scene* scene               = nullptr;
  GlobalStatePtr globalState = nullptr;
}; // end of struct ISceneExplorerComponentProps

class BABYLON_SHARED_EXPORT SceneGraphComponent {

public:
  /** Font Awesome Icon Unicodes **/
  static constexpr const char* faMinus = ICON_FA_MINUS;
  static constexpr const char* faPlus  = ICON_FA_PLUS;

public:
  SceneGraphComponent(const SceneGraphComponentProps& props);
  ~SceneGraphComponent();

  void reinitialize();
  void render();

  /**
   * @brief Deselects all children nodes of the node in the tree.
   */
  static void DeSelectAll(TreeNode<TreeItem>& node);

private:
  /**
   * @brief Initializes the "Nodes" subtree.
   */
  void _initializeNodesTreeItem(TreeNode<TreeItem>& parentTreeItem,
                                const NodePtr& node);

  TreeItem _createTreeItem(const char* label);
  TreeItem _createMaterialTreeItem(const MaterialPtr& material);
  TreeItem _createNodeTreeItem(const NodePtr& node);
  TreeItem _createSceneTreeItem(Scene* scene);
  TreeItem _createTextureTreeItem(const BaseTexturePtr& texture);

  float _calculateOffset(unsigned int nodeLevel);

  /**
   * @brief Render a selectable tree item.
   * @param node
   */
  void _renderSelectableTreeItem(TreeNode<TreeItem>& node);

  /**
   * @brief Renders the selectable children
   */
  void _renderChildren(TreeNode<TreeItem>& node);

  /**
   * @brief Render the scene graph.
   * @see https://github.com/ocornut/imgui/issues/1082
   */
  void _renderTree(TreeNode<TreeItem>& node);

  /**
   * @brief Notifies the selected entity in the scene graph.
   * @param entityInfo
   */
  void notifySelectionChange(EntityInfo& entityInfo);

private:
  SceneGraphComponentProps props;
  Tree<TreeItem> _sceneGraph;
  bool _initialized;
  std::function<bool(const TreeItem& a, const TreeItem& b)> _treeItemComparator;

}; // end of class SceneGraphComponent

} // end of namespace BABYLON

#endif // end of
       // BABYLON_INSPECTOR_COMPONENTS_SCENE_EXPLORER_SCENE_GRAPH_COMPONENT_H
