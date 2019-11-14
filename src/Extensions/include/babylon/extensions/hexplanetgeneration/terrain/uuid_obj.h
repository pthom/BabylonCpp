#ifndef BABYLON_EXTENSIONS_HEX_PLANET_GENERATION_TERRAIN_UUID_OBJ_H
#define BABYLON_EXTENSIONS_HEX_PLANET_GENERATION_TERRAIN_UUID_OBJ_H

#include <babylon/babylon_api.h>
#include <babylon/babylon_common.h>

namespace BABYLON {
namespace Extensions {

struct UidObj {
  UidObj(size_t id);
  virtual ~UidObj(); // = default

  virtual bool operator==(const UidObj& other) const;

  size_t id;
}; // end of struct UidObj

} // end of namespace Extensions
} // end of namespace BABYLON

#endif // end of BABYLON_EXTENSIONS_HEX_PLANET_GENERATION_TERRAIN_UUID_OBJ_H
