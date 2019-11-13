#include <babylon/meshes/csg/node.h>

#include <babylon/babylon_stl_util.h>
#include <babylon/meshes/csg/polygon.h>

namespace BABYLON {

CSG::Node::Node() : _plane{nullptr}, _front{nullptr}, _back{nullptr}
{
}

CSG::Node::Node(const std::vector<BABYLON::CSG::Polygon>& polygons)
    : _plane{nullptr}, _front{nullptr}, _back{nullptr}
{
  if (!polygons.empty()) {
    build(polygons);
  }
}

CSG::Node::~Node() = default;

std::unique_ptr<CSG::Node> CSG::Node::clone()
{
  auto node       = std::make_unique<Node>();
  node->_plane    = _plane ? _plane->cloneToNewObject() : nullptr;
  node->_front    = _front ? _front->clone() : nullptr;
  node->_back     = _back ? _back->clone() : nullptr;
  node->_polygons = _polygons;
  return node;
}

void CSG::Node::invert()
{
  for (auto& polygon : _polygons) {
    polygon.flip();
  }
  if (_plane) {
    _plane->flip();
  }
  if (_front) {
    _front->invert();
  }
  if (_back) {
    _back->invert();
  }
  std::swap(_front, _back);
}

std::vector<CSG::Polygon>
CSG::Node::clipPolygons(const std::vector<BABYLON::CSG::Polygon>& polygons)
{
  if (!_plane) {
    return polygons;
  }
  std::vector<Polygon> front, back;
  for (auto& polygon : polygons) {
    _plane->splitPolygon(polygon, front, back, front, back);
  }
  if (_front) {
    front = _front->clipPolygons(front);
  }
  if (_back) {
    back = _back->clipPolygons(back);
  }
  else {
    back.clear();
  }
  stl_util::concat(front, back);
  return front;
}

void BABYLON::CSG::Node::clipTo(BABYLON::CSG::Node& bsp)
{
  _polygons = bsp.clipPolygons(_polygons);
  if (_front) {
    _front->clipTo(bsp);
  }
  if (_back) {
    _back->clipTo(bsp);
  }
}

std::vector<BABYLON::CSG::Polygon> BABYLON::CSG::Node::allPolygons()
{
  std::vector<Polygon> polygons = _polygons;
  if (_front) {
    polygons = stl_util::concat(polygons, _front->allPolygons());
  }
  if (_back) {
    polygons = stl_util::concat(polygons, _back->allPolygons());
  }
  return polygons;
}

void BABYLON::CSG::Node::build(
  const std::vector<BABYLON::CSG::Polygon>& polygons)
{
  if (polygons.empty()) {
    return;
  }
  if (!_plane) {
    _plane = polygons[0].plane.second.cloneToNewObject();
  }
  std::vector<Polygon> front, back;
  for (auto& polygon : polygons) {
    _plane->splitPolygon(polygon, _polygons, _polygons, front, back);
  }
  if (!front.empty()) {
    if (!_front) {
      _front = std::make_unique<Node>();
    }
    _front->build(front);
  }
  if (!back.empty()) {
    if (!_back) {
      _back = std::make_unique<Node>();
    }
    _back->build(back);
  }
}

} // end of namespace BABYLON
