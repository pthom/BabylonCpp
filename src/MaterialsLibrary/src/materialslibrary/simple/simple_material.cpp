#include <babylon/materialslibrary/simple/simple_material.h>

#include <nlohmann/json.hpp>

#include <babylon/cameras/camera.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/materials/effect.h>
#include <babylon/materials/effect_creation_options.h>
#include <babylon/materials/effect_fallbacks.h>
#include <babylon/materials/material_helper.h>
#include <babylon/materials/standard_material.h>
#include <babylon/materials/textures/texture.h>
#include <babylon/materialslibrary/simple/simple_fragment_fx.h>
#include <babylon/materialslibrary/simple/simple_vertex_fx.h>
#include <babylon/meshes/abstract_mesh.h>
#include <babylon/meshes/mesh.h>
#include <babylon/meshes/sub_mesh.h>
#include <babylon/meshes/vertex_buffer.h>

namespace BABYLON {
namespace MaterialsLibrary {

SimpleMaterial::SimpleMaterial(const std::string& iName, Scene* scene)
    : PushMaterial{iName, scene}
    , diffuseTexture{this, &SimpleMaterial::get_diffuseTexture,
                     &SimpleMaterial::set_diffuseTexture}
    , diffuseColor{Color3(1.f, 1.f, 1.f)}
    , disableLighting{this, &SimpleMaterial::get_disableLighting,
                      &SimpleMaterial::set_disableLighting}
    , maxSimultaneousLights{this, &SimpleMaterial::get_maxSimultaneousLights,
                            &SimpleMaterial::set_maxSimultaneousLights}
    , _diffuseTexture{nullptr}
    , _disableLighting{false}
    , _maxSimultaneousLights{4}
    , _renderId{-1}
{
  // Vertex shader
  Effect::ShadersStore()["simpleVertexShader"] = simpleVertexShader;

  // Fragment shader
  Effect::ShadersStore()["simplePixelShader"] = simplePixelShader;
}

SimpleMaterial::~SimpleMaterial() = default;

TexturePtr& SimpleMaterial::get_diffuseTexture()
{
  return _diffuseTexture;
}

void SimpleMaterial::set_diffuseTexture(const TexturePtr& value)
{
  if (_diffuseTexture != value) {
    _diffuseTexture = value;
    _markAllSubMeshesAsTexturesDirty();
  }
}

bool SimpleMaterial::get_disableLighting() const
{
  return _disableLighting;
}

void SimpleMaterial::set_disableLighting(bool value)
{
  if (_disableLighting != value) {
    _disableLighting = value;
    _markAllSubMeshesAsLightsDirty();
  }
}

unsigned int SimpleMaterial::get_maxSimultaneousLights() const
{
  return _maxSimultaneousLights;
}

void SimpleMaterial::set_maxSimultaneousLights(unsigned int value)
{
  if (_maxSimultaneousLights != value) {
    _maxSimultaneousLights = value;
    _markAllSubMeshesAsLightsDirty();
  }
}

bool SimpleMaterial::needAlphaBlending() const
{
  return (alpha < 1.f);
}

bool SimpleMaterial::needAlphaTesting() const
{
  return false;
}

BaseTexturePtr SimpleMaterial::getAlphaTestTexture()
{
  return nullptr;
}

bool SimpleMaterial::isReadyForSubMesh(AbstractMesh* mesh, BaseSubMesh* subMesh,
                                       bool useInstances)
{
  if (isFrozen()) {
    if (_wasPreviouslyReady && subMesh->effect()) {
      return true;
    }
  }

  if (!subMesh->_materialDefines) {
    subMesh->_materialDefines = std::make_shared<SimpleMaterialDefines>();
  }

  auto definesPtr = std::static_pointer_cast<SimpleMaterialDefines>(
    subMesh->_materialDefines);
  auto& defines = *definesPtr.get();
  auto scene    = getScene();

  if (!checkReadyOnEveryCall && subMesh->effect()) {
    if (_renderId == scene->getRenderId()) {
      return true;
    }
  }

  auto engine = scene->getEngine();

  // Textures
  if (defines._areTexturesDirty) {
    defines._needUVs = false;
    if (scene->texturesEnabled()) {
      if (_diffuseTexture && StandardMaterial::DiffuseTextureEnabled()) {
        if (!_diffuseTexture->isReady()) {
          return false;
        }
        else {
          defines._needUVs           = true;
          defines.boolDef["DIFFUSE"] = true;
        }
      }
    }
  }

  // Misc.
  MaterialHelper::PrepareDefinesForMisc(mesh, scene, false, pointsCloud(),
                                        fogEnabled(),
                                        _shouldTurnAlphaTestOn(mesh), defines);

  // Lights
  defines._needNormals = MaterialHelper::PrepareDefinesForLights(
    scene, mesh, defines, false, _maxSimultaneousLights, _disableLighting);

  // Values that need to be evaluated on every frame
  MaterialHelper::PrepareDefinesForFrameBoundValues(
    scene, engine, defines, useInstances ? true : false);

  // Attribs
  MaterialHelper::PrepareDefinesForAttributes(mesh, defines, true, true);

  // Get correct effect
  if (defines.isDirty()) {
    defines.markAsProcessed();
    scene->resetCachedMaterial();

    // Fallbacks
    auto fallbacks = std::make_unique<EffectFallbacks>();
    if (defines["FOG"]) {
      fallbacks->addFallback(1, "FOG");
    }

    MaterialHelper::HandleFallbacksForShadows(defines, *fallbacks,
                                              maxSimultaneousLights());

    if (defines.intDef["NUM_BONE_INFLUENCERS"] > 0) {
      fallbacks->addCPUSkinningFallback(0, mesh);
    }

    // Attributes
    std::vector<std::string> attribs{VertexBuffer::PositionKind};

    if (defines["NORMAL"]) {
      attribs.emplace_back(VertexBuffer::NormalKind);
    }

    if (defines["UV1"]) {
      attribs.emplace_back(VertexBuffer::UVKind);
    }

    if (defines["UV2"]) {
      attribs.emplace_back(VertexBuffer::UV2Kind);
    }

    if (defines["VERTEXCOLOR"]) {
      attribs.emplace_back(VertexBuffer::ColorKind);
    }

    MaterialHelper::PrepareAttributesForBones(attribs, mesh, defines,
                                              *fallbacks);
    MaterialHelper::PrepareAttributesForInstances(attribs, defines);

    const std::string shaderName{"simple"};
    auto join = defines.toString();
    const std::vector<std::string> uniforms{
      "world",       "view",          "viewProjection", "vEyePosition",
      "vLightsType", "vDiffuseColor", "vFogInfos",      "vFogColor",
      "pointSize",   "vDiffuseInfos", "mBones",         "vClipPlane",
      "vClipPlane2", "vClipPlane3",   "vClipPlane4",    "diffuseMatrix"};
    const std::vector<std::string> samplers{"diffuseSampler"};
    const std::vector<std::string> uniformBuffers{};

    EffectCreationOptions options;
    options.attributes            = std::move(attribs);
    options.uniformsNames         = std::move(uniforms);
    options.uniformBuffersNames   = std::move(uniformBuffers);
    options.samplers              = std::move(samplers);
    options.materialDefines       = &defines;
    options.defines               = std::move(join);
    options.maxSimultaneousLights = maxSimultaneousLights();
    options.fallbacks             = std::move(fallbacks);
    options.onCompiled            = onCompiled;
    options.onError               = onError;
    options.indexParameters
      = {{"maxSimultaneousLights", maxSimultaneousLights()}};

    MaterialHelper::PrepareUniformsAndSamplersList(options);
    subMesh->setEffect(
      scene->getEngine()->createEffect(shaderName, options, engine),
      definesPtr);
  }

  if (!subMesh->effect() || !subMesh->effect()->isReady()) {
    return false;
  }

  _renderId           = scene->getRenderId();
  _wasPreviouslyReady = true;

  return true;
}

void SimpleMaterial::bindForSubMesh(Matrix& world, Mesh* mesh, SubMesh* subMesh)
{
  auto scene = getScene();

  auto defines
    = static_cast<SimpleMaterialDefines*>(subMesh->_materialDefines.get());
  if (!defines) {
    return;
  }

  auto effect = subMesh->effect();
  if (!effect) {
    return;
  }
  _activeEffect = effect;

  // Matrices
  bindOnlyWorldMatrix(world);
  _activeEffect->setMatrix("viewProjection", scene->getTransformMatrix());

  // Bones
  MaterialHelper::BindBonesParameters(mesh, _activeEffect);

  if (_mustRebind(scene, effect)) {
    // Textures
    if (_diffuseTexture && StandardMaterial::DiffuseTextureEnabled()) {
      _activeEffect->setTexture("diffuseSampler", _diffuseTexture);

      _activeEffect->setFloat2(
        "vDiffuseInfos", static_cast<float>(_diffuseTexture->coordinatesIndex),
        _diffuseTexture->level);
      _activeEffect->setMatrix("diffuseMatrix",
                               *_diffuseTexture->getTextureMatrix());
    }

    // Clip plane
    MaterialHelper::BindClipPlane(_activeEffect, scene);

    // Point size
    if (pointsCloud()) {
      _activeEffect->setFloat("pointSize", pointSize);
    }

    MaterialHelper::BindEyePosition(effect, scene);
  }

  _activeEffect->setColor4("vDiffuseColor", diffuseColor,
                           alpha * mesh->visibility);

  // Lights
  if (scene->lightsEnabled() && !_disableLighting) {
    MaterialHelper::BindLights(scene, mesh, _activeEffect, *defines,
                               maxSimultaneousLights());
  }

  // View
  if (scene->fogEnabled() && mesh->applyFog()
      && scene->fogMode() != Scene::FOGMODE_NONE) {
    _activeEffect->setMatrix("view", scene->getViewMatrix());
  }

  // Fog
  MaterialHelper::BindFogParameters(scene, mesh, _activeEffect);

  _afterBind(mesh, _activeEffect);
}

std::vector<IAnimatablePtr> SimpleMaterial::getAnimatables()
{
  std::vector<IAnimatablePtr> results;

  if (_diffuseTexture && _diffuseTexture->animations.size() > 0) {
    results.emplace_back(_diffuseTexture);
  }

  return results;
}

std::vector<BaseTexturePtr> SimpleMaterial::getActiveTextures() const
{
  auto activeTextures = PushMaterial::getActiveTextures();

  if (_diffuseTexture) {
    activeTextures.emplace_back(_diffuseTexture);
  }

  return activeTextures;
}

bool SimpleMaterial::hasTexture(const BaseTexturePtr& texture) const
{
  if (PushMaterial::hasTexture(texture)) {
    return true;
  }

  if (_diffuseTexture == texture) {
    return true;
  }

  return false;
}

void SimpleMaterial::dispose(bool forceDisposeEffect, bool forceDisposeTextures,
                             bool /*notBoundToMesh*/)
{
  if (_diffuseTexture) {
    _diffuseTexture->dispose();
  }

  PushMaterial::dispose(forceDisposeEffect, forceDisposeTextures);
}

MaterialPtr SimpleMaterial::clone(const std::string& /*name*/,
                                  bool /*cloneChildren*/) const
{
  return nullptr;
}

json SimpleMaterial::serialize() const
{
  return nullptr;
}

const std::string SimpleMaterial::getClassName() const
{
  return "SimpleMaterial";
}

SimpleMaterial* SimpleMaterial::Parse(const json& /*source*/, Scene* /*scene*/,
                                      const std::string& /*rootUrl*/)
{
  return nullptr;
}

} // end of namespace MaterialsLibrary
} // end of namespace BABYLON
