#include <babylon/inspector/components/sceneexplorer/entities/rendering_pipeline_tree_item_component.h>

#include <babylon/core/string.h>
#include <babylon/inspector/components/sceneexplorer/tree_item_label_component.h>
#include <babylon/postprocesses/renderpipeline/post_process_render_pipeline.h>
#include <imgui_utils/imgui_utils.h>

namespace BABYLON {

RenderingPipelineTreeItemComponent::RenderingPipelineTreeItemComponent(
  const IRenderPipelineItemComponentProps& iProps)
    : props{iProps}
{
  const auto& renderPipeline = props.renderPipeline;

  sprintf(label, "%s", "renderPipeline->name.c_str()");

  // Set the entity info
  const auto className = renderPipeline->getClassName();
  if (String::contains(className, "DefaultRenderingPipeline")) {
    entityInfo.type = EntityType::DefaultRenderingPipeline;
  }
  else if (String::contains(className, "LensRenderingPipeline")) {
    entityInfo.type = EntityType::LensRenderingPipeline;
  }
  else if (String::contains(className, "SSAORenderingPipeline")) {
    entityInfo.type = EntityType::SSAORenderingPipeline;
  }
  else if (String::contains(className, "Texture")) {
    entityInfo.type = EntityType::Texture;
  }
  else if (String::contains(className, "RenderingPipeline")) {
    entityInfo.type = EntityType::RenderingPipeline;
  }
}

RenderingPipelineTreeItemComponent::~RenderingPipelineTreeItemComponent() = default;

void RenderingPipelineTreeItemComponent::render()
{
  // TransformNode tree item label
  TreeItemLabelComponent::render(label, faMagic, ImGui::orangered);
}

} // end of namespace BABYLON
