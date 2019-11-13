#include <babylon/gamepads/stick_values.h>

#include <utility>

namespace BABYLON {

StickValues::StickValues() : x{0.f}, y{0.f}
{
}

StickValues::StickValues(float iX, float iY) : x{iX}, y{iY}
{
}

StickValues::StickValues(const StickValues& other) : x{other.x}, y{other.y}
{
}

StickValues::StickValues(StickValues&& other) : x{std::move(other.x)}, y{std::move(other.y)}
{
}

StickValues& StickValues::operator=(const StickValues& other)
{
  if (&other != this) {
    x = other.x;
    y = other.y;
  }

  return *this;
}

StickValues& StickValues::operator=(StickValues&& other)
{
  if (&other != this) {
    x = std::move(other.x);
    y = std::move(other.y);
  }

  return *this;
}

StickValues::~StickValues() = default;

} // end of namespace BABYLON
