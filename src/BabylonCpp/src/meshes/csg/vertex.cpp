#include <babylon/meshes/csg/vertex.h>

#include <sstream>

namespace BABYLON {

CSG::Vertex::Vertex(const Vector3& iPos, const Vector3& iNormal,
                    const Vector2& iUv)
    : pos{iPos}, normal{iNormal}, uv{iUv}
{
}

CSG::Vertex::Vertex(const BABYLON::CSG::Vertex& otherVertex) = default;
CSG::Vertex::Vertex(BABYLON::CSG::Vertex&& otherVertex) = default;
CSG::Vertex& CSG::Vertex::operator=(const BABYLON::CSG::Vertex& otherVertex) = default;
CSG::Vertex& CSG::Vertex::operator=(BABYLON::CSG::Vertex&& otherVertex) = default;
CSG::Vertex::~Vertex() = default;

CSG::Vertex CSG::Vertex::clone() const
{
  return Vertex(*this);
}

std::unique_ptr<CSG::Vertex> CSG::Vertex::cloneToNewObject() const
{
  return std::make_unique<Vertex>(*this);
}

namespace CSG {
std::ostream& operator<<(std::ostream& os, const BABYLON::CSG::Vertex& vertex)
{
  os << "{\"Position\":" << vertex.pos << ",\"Normal\":" << vertex.normal
     << ",\"UV\":" << vertex.uv << "}";
  return os;
}
} // namespace CSG

std::string CSG::Vertex::toString() const
{
  std::ostringstream oss;
  oss << (*this);
  return oss.str();
}

void CSG::Vertex::flip()
{
  normal = normal.scale(-1.f);
}

CSG::Vertex CSG::Vertex::interpolate(const BABYLON::CSG::Vertex& other, float t)
{
  return Vertex(Vector3::Lerp(pos, other.pos, t),
                Vector3::Lerp(normal, other.normal, t),
                Vector2::Lerp(uv, other.uv, t));
}

} // end of namespace BABYLON
