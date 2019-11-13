#include <babylon/misc/environment_texture_info.h>

#include <babylon/core/json_util.h>

namespace BABYLON {

EnvironmentTextureInfo::EnvironmentTextureInfo()
    : version{0}, width{0}, irradiance{nullptr}, specular{nullptr}
{
}

EnvironmentTextureInfo::EnvironmentTextureInfo(
  const EnvironmentTextureInfo& other)
    : version{other.version}
    , width{other.width}
    , irradiance{other.irradiance}
    , specular{other.specular}
{
}

EnvironmentTextureInfo::EnvironmentTextureInfo(EnvironmentTextureInfo&& other)
    : version{std::move(other.version)}
    , width{std::move(other.width)}
    , irradiance{std::move(other.irradiance)}
    , specular{std::move(other.specular)}
{
}

EnvironmentTextureInfo& EnvironmentTextureInfo::
operator=(const EnvironmentTextureInfo& other)
{
  if (&other != this) {
    version    = other.version;
    width      = other.width;
    irradiance = other.irradiance;
    specular   = other.specular;
  }

  return *this;
}

EnvironmentTextureInfo& EnvironmentTextureInfo::
operator=(EnvironmentTextureInfo&& other)
{
  if (&other != this) {
    version    = std::move(other.version);
    width      = std::move(other.width);
    irradiance = std::move(other.irradiance);
    specular   = std::move(other.specular);
  }

  return *this;
}

EnvironmentTextureInfo::~EnvironmentTextureInfo() = default;

EnvironmentTextureInfoPtr
EnvironmentTextureInfo::Parse(const json& parsedManifest)
{
  EnvironmentTextureInfo manifest;
  manifest.version = json_util::get_number(parsedManifest, "version", 0u);
  manifest.width   = json_util::get_number(parsedManifest, "width", 0);

  // Irradiance information
  if (json_util::has_key(parsedManifest, "irradiance")
      && parsedManifest["irradiance"].is_object()) {
    auto parsedIrradiance = parsedManifest["irradiance"];
    manifest.irradiance
      = EnvironmentTextureIrradianceInfoV1::Parse(parsedIrradiance);
  }

  // Specular information
  if (json_util::has_key(parsedManifest, "specular")
      && parsedManifest["specular"].is_object()) {
    auto parsedSpecular = parsedManifest["specular"];
    manifest.specular = EnvironmentTextureSpecularInfoV1::Parse(parsedSpecular);
  }

  return std::make_shared<EnvironmentTextureInfo>(manifest);
}

} // end of namespace BABYLON
