#include <babylon/materials/shader_material.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/bones/skeleton.h>
#include <babylon/core/string.h>
#include <babylon/engines/engine.h>
#include <babylon/engines/scene.h>
#include <babylon/materials/effect.h>
#include <babylon/materials/effect_creation_options.h>
#include <babylon/materials/effect_fallbacks.h>
#include <babylon/materials/material_helper.h>
#include <babylon/materials/textures/texture.h>
#include <babylon/math/color3.h>
#include <babylon/math/color4.h>
#include <babylon/math/vector2.h>
#include <babylon/math/vector3.h>
#include <babylon/math/vector4.h>
#include <babylon/meshes/abstract_mesh.h>
#include <babylon/meshes/mesh.h>
#include <babylon/meshes/vertex_buffer.h>
#include <babylon/misc/tools.h>

namespace BABYLON {

ShaderMaterial::ShaderMaterial(const std::string& iName, Scene* scene,
                               const std::string& shaderPath,
                               const IShaderMaterialOptions& options)
    : Material{iName, scene}, _shaderPath{shaderPath}, _renderId{-1}
{
  _options.needAlphaBlending = options.needAlphaBlending;
  _options.needAlphaTesting  = options.needAlphaTesting;
  _options.attributes        = options.attributes;
  _options.uniforms          = options.uniforms;
  _options.uniformBuffers    = options.uniformBuffers;
  _options.samplers          = options.samplers;
  _options.defines           = options.defines;
}

ShaderMaterial::~ShaderMaterial() = default;

IShaderMaterialOptions& ShaderMaterial::options()
{
  return _options;
}

const std::string ShaderMaterial::getClassName() const
{
  return "ShaderMaterial";
}

Type ShaderMaterial::type() const
{
  return Type::SHADERMATERIAL;
}

bool ShaderMaterial::needAlphaBlending() const
{
  return (alpha < 1.f) || _options.needAlphaBlending;
}

bool ShaderMaterial::needAlphaTesting() const
{
  return _options.needAlphaTesting;
}

void ShaderMaterial::_checkUniform(const std::string& uniformName)
{
  if (!stl_util::contains(_options.uniforms, uniformName)) {
    _options.uniforms.emplace_back(uniformName);
  }
}

ShaderMaterial& ShaderMaterial::setTexture(const std::string& iName,
                                           const TexturePtr& texture)
{
  if (!stl_util::contains(_options.samplers, iName)) {
    _options.samplers.emplace_back(iName);
  }
  _textures[iName] = texture;

  return *this;
}

ShaderMaterial&
ShaderMaterial::setTextureArray(const std::string& iName,
                                const std::vector<BaseTexturePtr>& textures)
{
  if (!stl_util::contains(_options.samplers, iName)) {
    _options.samplers.emplace_back(iName);
  }

  _checkUniform(iName);

  _textureArrays[iName] = textures;

  return *this;
}

ShaderMaterial& ShaderMaterial::setInt(const std::string& iName, int value)
{
  _checkUniform(iName);
  _ints[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setFloat(const std::string& iName, float value)
{
  _checkUniform(iName);
  _floats[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setFloats(const std::string& iName,
                                          const Float32Array& value)
{
  _checkUniform(iName);
  _floatsArrays[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setColor3(const std::string& iName,
                                          const Color3& value)
{
  _checkUniform(iName);
  _colors3[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setColor3Array(const std::string& iName,
                                               const std::vector<Color3>& value)
{
  _checkUniform(iName);
  _colors3Arrays[iName] = {};
  for (const auto& color : value) {
    Float32Array arr(3);
    color.toArray(arr, 0);
    stl_util::concat(_colors3Arrays[iName], arr);
  }

  return *this;
}

ShaderMaterial& ShaderMaterial::setColor4(const std::string& iName,
                                          const Color4& value)
{
  _checkUniform(iName);
  _colors4[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setColor4Array(const std::string& iName,
                                               const std::vector<Color4>& value)
{
  _checkUniform(iName);
  _colors4Arrays[iName] = {};
  for (const auto& color : value) {
    Float32Array arr(4);
    color.toArray(arr, 0);
    stl_util::concat(_colors4Arrays[iName], arr);
  }

  return *this;
}

ShaderMaterial& ShaderMaterial::setVector2(const std::string& iName,
                                           const Vector2& value)
{
  _checkUniform(iName);
  _vectors2[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setVector3(const std::string& iName,
                                           const Vector3& value)
{
  _checkUniform(iName);
  _vectors3[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setVector4(const std::string& iName,
                                           const Vector4& value)
{
  _checkUniform(iName);
  _vectors4[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setMatrix(const std::string& iName,
                                          const Matrix& value)
{
  _checkUniform(iName);
  _matrices[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setMatrix3x3(const std::string& iName,
                                             const Float32Array& value)
{
  _checkUniform(iName);
  _matrices3x3[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setMatrix2x2(const std::string& iName,
                                             const Float32Array& value)
{
  _checkUniform(iName);
  _matrices2x2[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setArray2(const std::string& iName,
                                          const Float32Array& value)
{
  _checkUniform(iName);
  _vectors2Arrays[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setArray3(const std::string& iName,
                                          const Float32Array& value)
{
  _checkUniform(iName);
  _vectors3Arrays[iName] = value;

  return *this;
}

ShaderMaterial& ShaderMaterial::setArray4(const std::string& iName,
                                          const Float32Array& value)
{
  _checkUniform(iName);
  _vectors4Arrays[iName] = value;

  return *this;
}

bool ShaderMaterial::_checkCache(AbstractMesh* mesh, bool useInstances)
{
  if (!mesh) {
    return true;
  }

  if (_effect
      && (_effect->defines.find("#define INSTANCES") != std::string::npos)
           != useInstances) {
    return false;
  }

  return false;
}

bool ShaderMaterial::isReadyForSubMesh(AbstractMesh* mesh,
                                       BaseSubMesh* /*subMesh*/,
                                       bool useInstances)
{
  return isReady(mesh, useInstances);
}

bool ShaderMaterial::isReady(AbstractMesh* mesh, bool useInstances)
{
  auto scene  = getScene();
  auto engine = scene->getEngine();

  if (!checkReadyOnEveryCall) {
    if (_renderId == scene->getRenderId()) {
      if (_checkCache(mesh, useInstances)) {
        return true;
      }
    }
  }

  // Instances
  std::vector<std::string> defines;
  std::vector<std::string> attribs;
  auto fallbacks = std::make_unique<EffectFallbacks>();

  for (const auto& _define : _options.defines) {
    defines.emplace_back(_define);
  }

  for (const auto& _attrib : _options.attributes) {
    attribs.emplace_back(_attrib);
  }

  if (mesh && mesh->isVerticesDataPresent(VertexBuffer::ColorKind)) {
    attribs.emplace_back(VertexBuffer::ColorKind);
    defines.emplace_back("#define VERTEXCOLOR");
  }

  if (useInstances) {
    defines.emplace_back("#define INSTANCES");
    MaterialHelper::PushAttributesForInstances(attribs);
  }

  // Bones
  if (mesh && mesh->useBones() && mesh->computeBonesUsingShaders()
      && mesh->skeleton()) {
    attribs.emplace_back(VertexBuffer::MatricesIndicesKind);
    attribs.emplace_back(VertexBuffer::MatricesWeightsKind);
    if (mesh->numBoneInfluencers() > 4) {
      attribs.emplace_back(VertexBuffer::MatricesIndicesExtraKind);
      attribs.emplace_back(VertexBuffer::MatricesWeightsExtraKind);
    }

    const auto& skeleton = mesh->skeleton();

    defines.emplace_back("#define NUM_BONE_INFLUENCERS "
                         + std::to_string(mesh->numBoneInfluencers()));
    fallbacks->addCPUSkinningFallback(0, mesh);

    if (skeleton->isUsingTextureForMatrices()) {
      defines.emplace_back("#define BONETEXTURE");

      if (!stl_util::contains(_options.uniforms, "boneTextureWidth")) {
        _options.uniforms.emplace_back("boneTextureWidth");
      }

      if (!stl_util::contains(_options.samplers, "boneSampler")) {
        _options.samplers.emplace_back("boneSampler");
      }
    }
    else {
      defines.emplace_back("#define BonesPerMesh "
                           + std::to_string(skeleton->bones.size() + 1));

      if (!stl_util::contains(_options.uniforms, "mBones")) {
        _options.uniforms.emplace_back("mBones");
      }
    }
  }
  else {
    defines.emplace_back("#define NUM_BONE_INFLUENCERS 0");
  }

  // Textures
  for (const auto& item : _textures) {
    if (!item.second->isReady()) {
      return false;
    }
  }

  // Alpha test
  if (mesh && _shouldTurnAlphaTestOn(mesh)) {
    defines.emplace_back("#define ALPHATEST");
  }

  auto previousEffect = _effect;
  auto join           = String::join(defines, '\n');

  EffectCreationOptions options;
  options.attributes          = std::move(attribs);
  options.uniformsNames       = _options.uniforms;
  options.uniformBuffersNames = _options.uniformBuffers;
  options.samplers            = _options.samplers;
  options.defines             = std::move(join);
  options.fallbacks           = std::move(fallbacks);
  options.onCompiled          = onCompiled;
  options.onError             = onError;

  _effect = engine->createEffect(_shaderPath, options, engine);

  if (!_effect->isReady()) {
    return false;
  }

  if (previousEffect != _effect) {
    scene->resetCachedMaterial();
  }

  _renderId = scene->getRenderId();

  return true;
}

void ShaderMaterial::bindOnlyWorldMatrix(Matrix& world)
{
  auto scene = getScene();

  if (!_effect) {
    return;
  }

  if (stl_util::contains(_options.uniforms, "world")) {
    _effect->setMatrix("world", world);
  }

  if (stl_util::contains(_options.uniforms, "worldView")) {
    world.multiplyToRef(scene->getViewMatrix(), _cachedWorldViewMatrix);
    _effect->setMatrix("worldView", _cachedWorldViewMatrix);
  }

  if (stl_util::contains(_options.uniforms, "worldViewProjection")) {
    world.multiplyToRef(scene->getTransformMatrix(),
                        _cachedWorldViewProjectionMatrix);
    _effect->setMatrix("worldViewProjection",
                       world.multiply(_cachedWorldViewProjectionMatrix));
  }
}

void ShaderMaterial::bind(Matrix& world, Mesh* mesh)
{
  // Std values
  bindOnlyWorldMatrix(world);

  if (_effect && getScene()->getCachedMaterial() != this) {
    if (stl_util::contains(_options.uniforms, "view")) {
      _effect->setMatrix("view", getScene()->getViewMatrix());
    }

    if (stl_util::contains(_options.uniforms, "projection")) {
      _effect->setMatrix("projection", getScene()->getProjectionMatrix());
    }

    if (stl_util::contains(_options.uniforms, "viewProjection")) {
      _effect->setMatrix("viewProjection", getScene()->getTransformMatrix());
    }

    // Bones
    MaterialHelper::BindBonesParameters(mesh, _effect);

    // Texture
    for (const auto& kv : _textures) {
      _effect->setTexture(kv.first, kv.second);
    }

    // Texture arrays
    for (const auto& kv : _textureArrays) {
      _effect->setTextureArray(kv.first, kv.second);
    }

    // Int
    for (const auto& kv : _ints) {
      _effect->setInt(kv.first, kv.second);
    }

    // Float
    for (const auto& kv : _floats) {
      _effect->setFloat(kv.first, kv.second);
    }

    // Floats
    for (const auto& kv : _floatsArrays) {
      _effect->setArray(kv.first, kv.second);
    }

    // Color3
    for (const auto& kv : _colors3) {
      _effect->setColor3(kv.first, kv.second);
    }

    // Color3Array
    for (const auto& kv : _colors3Arrays) {
      _effect->setArray3(kv.first, kv.second);
    }

    // Color4
    for (const auto& kv : _colors4) {
      const Color4& color = kv.second;
      _effect->setFloat4(kv.first, color.r, color.g, color.b, color.a);
    }

    // Color4Array
    for (const auto& kv : _colors4Arrays) {
      _effect->setArray4(kv.first, kv.second);
    }

    // Vector2
    for (const auto& kv : _vectors2) {
      _effect->setVector2(kv.first, kv.second);
    }

    // Vector3
    for (const auto& kv : _vectors3) {
      _effect->setVector3(kv.first, kv.second);
    }

    // Vector4
    for (const auto& kv : _vectors4) {
      _effect->setVector4(kv.first, kv.second);
    }

    // Matrix
    for (const auto& kv : _matrices) {
      _effect->setMatrix(kv.first, kv.second);
    }

    // Matrix 3x3
    for (const auto& kv : _matrices3x3) {
      _effect->setMatrix3x3(kv.first, kv.second);
    }

    // Matrix 2x2
    for (const auto& kv : _matrices2x2) {
      _effect->setMatrix2x2(kv.first, kv.second);
    }

    // Vector2Array
    for (const auto& kv : _vectors2Arrays) {
      _effect->setArray2(kv.first, kv.second);
    }

    // Vector3Array
    for (const auto& kv : _vectors3Arrays) {
      _effect->setArray3(kv.first, kv.second);
    }

    // Vector4Array
    for (const auto& kv : _vectors4Arrays) {
      _effect->setArray4(kv.first, kv.second);
    }
  }

  _afterBind(mesh);
}

std::vector<BaseTexturePtr> ShaderMaterial::getActiveTextures() const
{
  auto activeTextures = Material::getActiveTextures();

  for (const auto& textureItem : _textures) {
    activeTextures.emplace_back(textureItem.second);
  }

  for (const auto& textureArrayItem : _textureArrays) {
    const auto& array = textureArrayItem.second;
    for (std::size_t index = 0; index < array.size(); ++index) {
      activeTextures.emplace_back(array[index]);
    }
  }

  return activeTextures;
}

bool ShaderMaterial::hasTexture(const BaseTexturePtr& texture) const
{
  if (Material::hasTexture(texture)) {
    return true;
  }

  auto it1
    = std::find_if(_textures.begin(), _textures.end(),
                   [&texture](const std::pair<std::string, TexturePtr>& tex) {
                     return tex.second == texture;
                   });
  if (it1 != _textures.end()) {
    return true;
  }

  auto it2 = std::find_if(
    _textureArrays.begin(), _textureArrays.end(),
    [&texture](
      const std::pair<std::string, std::vector<BaseTexturePtr>>& textures) {
      return stl_util::contains(textures.second, texture);
    });
  if (it2 != _textureArrays.end()) {
    return true;
  }

  return false;
}

MaterialPtr ShaderMaterial::clone(const std::string& iName,
                                  bool /*cloneChildren*/) const
{
  return ShaderMaterial::New(iName, getScene(), _shaderPath, _options);
}

void ShaderMaterial::dispose(bool forceDisposeEffect, bool forceDisposeTextures,
                             bool notBoundToMesh)
{
  if (forceDisposeTextures) {
    for (const auto& kv : _textures) {
      kv.second->dispose();
    }

    for (const auto& kv : _textureArrays) {
      for (const auto& textureArray : kv.second) {
        textureArray->dispose();
      }
    }
  }

  _textures.clear();

  Material::dispose(forceDisposeEffect, forceDisposeTextures, notBoundToMesh);
}

json ShaderMaterial::serialize() const
{
  return nullptr;
}

std::unique_ptr<ShaderMaterial>
ShaderMaterial::Parse(const json& /*source*/, Scene* /*scene*/,
                      const std::string& /*url*/)
{
  return nullptr;
}

} // end of namespace BABYLON
