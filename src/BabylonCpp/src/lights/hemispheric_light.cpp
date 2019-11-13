#include <babylon/lights/hemispheric_light.h>

#include <nlohmann/json.hpp>

#include <babylon/engines/scene.h>
#include <babylon/materials/effect.h>
#include <babylon/materials/material_defines.h>
#include <babylon/materials/uniform_buffer.h>

namespace BABYLON {

bool HemisphericLight::NodeConstructorAdded = false;

void HemisphericLight::AddNodeConstructor()
{
  Node::AddNodeConstructor(
    "Light_Type_3", [](const std::string& iName, Scene* scene,
                       const std::optional<json>& /*options*/) {
      return HemisphericLight::New(iName, Vector3::Zero(), scene);
    });
  HemisphericLight::NodeConstructorAdded = true;
}

HemisphericLight::HemisphericLight(const std::string& iName, Scene* scene)
    : HemisphericLight(iName, Vector3::Up(), scene)
{
}

HemisphericLight::HemisphericLight(const std::string& iName,
                                   const Vector3& iDirection, Scene* scene)
    : Light{iName, scene}
    , groundColor{Color3(0.f, 0.f, 0.f)}
    , direction{iDirection}
{
}

HemisphericLight::~HemisphericLight() = default;

void HemisphericLight::_buildUniformLayout()
{
  _uniformBuffer->addUniform("vLightData", 4);
  _uniformBuffer->addUniform("vLightDiffuse", 4);
  _uniformBuffer->addUniform("vLightSpecular", 3);
  _uniformBuffer->addUniform("vLightGround", 3);
  _uniformBuffer->addUniform("shadowsInfo", 3);
  _uniformBuffer->addUniform("depthValues", 2);
  _uniformBuffer->create();
}

const std::string HemisphericLight::getClassName() const
{
  return "HemisphericLight";
}

Type HemisphericLight::type() const
{
  return Type::HEMISPHERICLIGHT;
}

Vector3 HemisphericLight::setDirectionToTarget(const Vector3& target)
{
  direction = Vector3::Normalize(target.subtract(Vector3::Zero()));
  return direction;
}

IShadowGeneratorPtr HemisphericLight::getShadowGenerator()
{
  return nullptr;
}

void HemisphericLight::transferToEffect(const EffectPtr& /*effect*/,
                                        const std::string& lightIndex)
{
  auto normalizeDirection = Vector3::Normalize(direction);

  _uniformBuffer->updateFloat4("vLightData",         //
                               normalizeDirection.x, //
                               normalizeDirection.y, //
                               normalizeDirection.z, //
                               0.f,                  //
                               lightIndex);

  _uniformBuffer->updateColor3("vLightGround", groundColor.scale(intensity),
                               lightIndex);
}

HemisphericLight& HemisphericLight::transferToNodeMaterialEffect(
  const EffectPtr& effect, const std::string& lightDataUniformName)
{
  const auto normalizeDirection = Vector3::Normalize(direction);
  effect->setFloat3(lightDataUniformName, normalizeDirection.x,
                    normalizeDirection.y, normalizeDirection.z);
  return *this;
}

Matrix& HemisphericLight::computeWorldMatrix(bool /*force*/,
                                             bool /*useWasUpdatedFlag*/)
{
  return _worldMatrix;
}

unsigned int HemisphericLight::getTypeID() const
{
  return Light::LIGHTTYPEID_HEMISPHERICLIGHT;
}

void HemisphericLight::prepareLightSpecificDefines(MaterialDefines& defines,
                                                   unsigned int lightIndex)
{
  defines.boolDef["HEMILIGHT" + std::to_string(lightIndex)] = true;
}

} // end of namespace BABYLON
