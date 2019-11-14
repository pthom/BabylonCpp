#ifndef BABYLON_EXTENSIONS_HEX_PLANET_GENERATION_TERRAIN_HEIGHTMAP_H
#define BABYLON_EXTENSIONS_HEX_PLANET_GENERATION_TERRAIN_HEIGHTMAP_H

#include <memory>

#include <babylon/babylon_api.h>
#include <babylon/babylon_common.h>

namespace BABYLON {
namespace Extensions {

class FastSimplexNoise;

class Heightmap {

public:
  Heightmap(const std::string& seed, float maxHeight, float height,
            float width);
  ~Heightmap(); // = default

  void generateHeightmap();
  float getHeight(float u, float v) const;
  float getHeightNormalized(float u, float v) const;

private:
  std::unique_ptr<FastSimplexNoise> _noise;
  std::string _seed;
  Float32Array _data;
  float _width;
  float _height;
  float _maxHeight;

}; // end of class Heightmap

} // end of namespace Extensions
} // end of namespace BABYLON

#endif // end of
       // BABYLON_EXTENSIONS_HEX_PLANET_GENERATION_TERRAIN_HEIGHTMAP_H
