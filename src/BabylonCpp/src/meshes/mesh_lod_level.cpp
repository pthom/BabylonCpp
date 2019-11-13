#include <babylon/meshes/mesh_lod_level.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/meshes/mesh.h>

namespace BABYLON {

MeshLODLevel::MeshLODLevel(float iDistance, const MeshPtr& iMesh)
    : distance{iDistance}, mesh{iMesh}
{
}

MeshLODLevel::MeshLODLevel(const MeshLODLevel& other)
    : distance{other.distance}, mesh{other.mesh}
{
}

MeshLODLevel::MeshLODLevel(MeshLODLevel&& other)
    : distance{std::move(other.distance)}, mesh{std::move(other.mesh)}
{
}

MeshLODLevel& MeshLODLevel::operator=(const MeshLODLevel& other)
{
  if (&other != this) {
    distance = other.distance;
    mesh     = other.mesh;
  }

  return *this;
}

MeshLODLevel& MeshLODLevel::operator=(MeshLODLevel&& other)
{
  if (&other != this) {
    distance = std::move(other.distance);
    mesh     = std::move(other.mesh);
  }

  return *this;
}

MeshLODLevel::~MeshLODLevel() = default;

bool MeshLODLevel::operator==(const MeshLODLevel& other) const
{
  return stl_util::almost_equal(distance, other.distance)
         && (mesh == other.mesh);
}

bool MeshLODLevel::operator!=(const MeshLODLevel& other) const
{
  return !(operator==(other));
}

} // end of namespace BABYLON
