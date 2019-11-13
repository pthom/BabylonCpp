#include <babylon/meshes/ground_mesh.h>

#include <babylon/collisions/picking_info.h>
#include <babylon/core/json_util.h>
#include <babylon/culling/bounding_box.h>
#include <babylon/culling/bounding_info.h>
#include <babylon/culling/ray.h>
#include <babylon/math/tmp_vectors.h>
#include <babylon/meshes/vertex_buffer.h>

namespace BABYLON {

GroundMesh::GroundMesh(const std::string& iName, Scene* scene)
    : Mesh{iName, scene}
    , generateOctree{false}
    , subdivisions{this, &GroundMesh::get_subdivisions}
    , subdivisionsX{this, &GroundMesh::get_subdivisionsX}
    , subdivisionsY{this, &GroundMesh::get_subdivisionsY}
{
}

GroundMesh::~GroundMesh() = default;

const std::string GroundMesh::getClassName() const
{
  return "GroundMesh";
}

Type GroundMesh::type() const
{
  return Type::GROUNDMESH;
}

size_t GroundMesh::get_subdivisions() const
{
  return std::min(_subdivisionsX, _subdivisionsY);
}

size_t GroundMesh::get_subdivisionsX() const
{
  return _subdivisionsX;
}

size_t GroundMesh::get_subdivisionsY() const
{
  return _subdivisionsY;
}

void GroundMesh::optimize(size_t chunksCount, size_t octreeBlocksSize)
{
  _subdivisionsX = chunksCount;
  _subdivisionsY = chunksCount;
  subdivide(chunksCount);
  createOrUpdateSubmeshesOctree(octreeBlocksSize);
}

float GroundMesh::getHeightAtCoordinates(float x, float z)
{
  auto world   = getWorldMatrix();
  auto& invMat = TmpVectors::MatrixArray[5];
  world.invertToRef(invMat);
  auto& tmpVect = TmpVectors::Vector3Array[8];
  // transform x,z in the mesh local space
  Vector3::TransformCoordinatesFromFloatsToRef(x, 0.f, z, invMat, tmpVect);
  x = tmpVect.x;
  z = tmpVect.z;
  if (x < _minX || x > _maxX || z < _minZ || z > _maxZ) {
    return position().y;
  }
  if (_heightQuads.empty()) {
    _initHeightQuads();
    _computeHeightQuads();
  }
  auto facet = _getFacetAt(x, z);
  auto y     = -(facet.x * x + facet.z * z + facet.w) / facet.y;
  // return y in the World system
  Vector3::TransformCoordinatesFromFloatsToRef(0.f, y, 0.f, world, tmpVect);
  return tmpVect.y;
}

Vector3 GroundMesh::getNormalAtCoordinates(float x, float z)
{
  Vector3 normal(0.f, 1.f, 0.f);
  getNormalAtCoordinatesToRef(x, z, normal);
  return normal;
}

GroundMesh& GroundMesh::getNormalAtCoordinatesToRef(float x, float z,
                                                    Vector3& ref)
{
  auto world   = getWorldMatrix();
  auto& tmpMat = TmpVectors::MatrixArray[5];
  world.invertToRef(tmpMat);
  auto& tmpVect = TmpVectors::Vector3Array[8];
  // transform x,z in the mesh local space
  Vector3::TransformCoordinatesFromFloatsToRef(x, 0.f, z, tmpMat, tmpVect);
  x = tmpVect.x;
  z = tmpVect.z;
  if (x < _minX || x > _maxX || z < _minZ || z > _maxZ) {
    return *this;
  }
  if (_heightQuads.empty()) {
    _initHeightQuads();
    _computeHeightQuads();
  }
  auto facet = _getFacetAt(x, z);
  Vector3::TransformNormalFromFloatsToRef(facet.x, facet.y, facet.z, world,
                                          ref);
  return *this;
}

GroundMesh& GroundMesh::updateCoordinateHeights()
{
  if (_heightQuads.empty()) {
    _initHeightQuads();
  }
  _computeHeightQuads();
  return *this;
}

Vector4 GroundMesh::_getFacetAt(float x, float z)
{
  // retrieve col and row from x, z coordinates in the ground local system
  auto subdivisionsXf = static_cast<float>(_subdivisionsX);
  auto subdivisionsYf = static_cast<float>(_subdivisionsY);
  auto col
    = static_cast<size_t>(std::floor((x + _maxX) * subdivisionsXf / _width));
  auto row = static_cast<size_t>(
    std::floor(-(z + _maxZ) * subdivisionsYf / _height + subdivisionsYf));
  auto& quad = _heightQuads[row * _subdivisionsX + col];
  Vector4 facet;
  if (z < quad.slope.x * x + quad.slope.y) {
    facet = quad.facet1;
  }
  else {
    facet = quad.facet2;
  }
  return facet;
}

GroundMesh& GroundMesh::_initHeightQuads()
{
  for (size_t row = 0; row < _subdivisionsX; ++row) {
    for (size_t col = 0; col < _subdivisionsY; ++col) {
      Quad quad;
      quad.slope  = Vector2::Zero();
      quad.facet1 = Vector4(0.f, 0.f, 0.f, 0.f);
      quad.facet2 = Vector4(0.f, 0.f, 0.f, 0.f);

      _heightQuads[row * _subdivisionsX + col] = std::move(quad);
    }
  }
  return *this;
}

GroundMesh& GroundMesh::_computeHeightQuads()
{
  auto positions = getVerticesData(VertexBuffer::PositionKind);

  if (positions.empty()) {
    return *this;
  }

  auto& v1    = TmpVectors::Vector3Array[3];
  auto& v2    = TmpVectors::Vector3Array[2];
  auto& v3    = TmpVectors::Vector3Array[1];
  auto& v4    = TmpVectors::Vector3Array[0];
  auto& v1v2  = TmpVectors::Vector3Array[4];
  auto& v1v3  = TmpVectors::Vector3Array[5];
  auto& v1v4  = TmpVectors::Vector3Array[6];
  auto& norm1 = TmpVectors::Vector3Array[7];
  auto& norm2 = TmpVectors::Vector3Array[8];
  size_t i    = 0;
  size_t j    = 0;
  size_t k    = 0;
  auto cd     = 0.f; // 2D slope coefficient : z = cd * x + h
  auto h      = 0.f;
  auto d1     = 0.f; // facet plane equation : ax + by + cz + d = 0
  auto d2     = 0.f;

  for (size_t row = 0; row < subdivisionsY; ++row) {
    for (size_t col = 0; col < _subdivisionsX; ++col) {
      i    = col * 3;
      j    = row * (_subdivisionsX + 1) * 3;
      k    = (row + 1) * (_subdivisionsX + 1) * 3;
      v1.x = positions[j + i];
      v1.y = positions[j + i + 1];
      v1.z = positions[j + i + 2];
      v2.x = positions[j + i + 3];
      v2.y = positions[j + i + 4];
      v2.z = positions[j + i + 5];
      v3.x = positions[k + i];
      v3.y = positions[k + i + 1];
      v3.z = positions[k + i + 2];
      v4.x = positions[k + i + 3];
      v4.y = positions[k + i + 4];
      v4.z = positions[k + i + 5];

      // 2D slope V1V4
      cd = (v4.z - v1.z) / (v4.x - v1.x);
      h  = v1.z - cd * v1.x; // v1 belongs to the slope

      // facet equations :
      // we compute each facet normal vector
      // the equation of the facet plane is : norm.x * x + norm.y * y + norm.z *
      // z + d = 0
      // we compute the value d by applying the equation to v1 which belongs to
      // the plane
      // then we store the facet equation in a Vector4
      v2.subtractToRef(v1, v1v2);
      v3.subtractToRef(v1, v1v3);
      v4.subtractToRef(v1, v1v4);
      Vector3::CrossToRef(v1v4, v1v3, norm1);
      Vector3::CrossToRef(v1v2, v1v4, norm2);
      norm1.normalize();
      norm2.normalize();
      d1 = -(norm1.x * v1.x + norm1.y * v1.y + norm1.z * v1.z);
      d2 = -(norm2.x * v2.x + norm2.y * v2.y + norm2.z * v2.z);

      auto& quad = _heightQuads[row * _subdivisionsX + col];
      quad.slope.copyFromFloats(cd, h);
      quad.facet1.copyFromFloats(norm1.x, norm1.y, norm1.z, d1);
      quad.facet2.copyFromFloats(norm2.x, norm2.y, norm2.z, d2);
    }
  }
  return *this;
}

void GroundMesh::serialize(json& /*serializationObject*/) const
{
}

GroundMeshPtr GroundMesh::Parse(const json& parsedMesh, Scene* scene)
{
  auto result
    = GroundMesh::New(json_util::get_string(parsedMesh, "name"), scene);

  result->_subdivisionsX
    = json_util::get_number<size_t>(parsedMesh, "subdivisionsX", 1);
  result->_subdivisionsY
    = json_util::get_number<size_t>(parsedMesh, "subdivisionsY", 1);

  result->_minX = json_util::get_number(parsedMesh, "minX", 1.f);
  result->_maxX = json_util::get_number(parsedMesh, "maxX", 1.f);

  result->_minZ = json_util::get_number(parsedMesh, "minZ", 1.f);
  result->_maxZ = json_util::get_number(parsedMesh, "maxZ", 1.f);

  result->_width  = json_util::get_number(parsedMesh, "width", 1.f);
  result->_height = json_util::get_number(parsedMesh, "height", 1.f);

  return result;
}

} // end of namespace BABYLON
