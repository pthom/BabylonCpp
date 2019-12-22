#include <babylon/misc/highdynamicrange/panorama_to_cube_map_tools.h>

#include <cmath>

#include <babylon/core/logging.h>
#include <babylon/engines/constants.h>

namespace BABYLON {

std::array<Vector3, 4> PanoramaToCubeMapTools::FACE_FRONT{{
  Vector3(-1.f, -1.f, -1.f), //
  Vector3(1.f, -1.f, -1.f),  //
  Vector3(-1.f, 1.f, -1.f),  //
  Vector3(1.f, 1.f, -1.f)    //
}};

std::array<Vector3, 4> PanoramaToCubeMapTools::FACE_BACK{{
  Vector3(1.f, -1.f, 1.f),  //
  Vector3(-1.f, -1.f, 1.f), //
  Vector3(1.f, 1.f, 1.f),   //
  Vector3(-1.f, 1.f, 1.f)   //
}};

std::array<Vector3, 4> PanoramaToCubeMapTools::FACE_RIGHT{{
  Vector3(1.f, -1.f, -1.f), //
  Vector3(1.f, -1.f, 1.f),  //
  Vector3(1.f, 1.f, -1.f),  //
  Vector3(1.f, 1.f, 1.f)    //
}};

std::array<Vector3, 4> PanoramaToCubeMapTools::FACE_LEFT{{
  Vector3(-1.f, -1.f, 1.f),  //
  Vector3(-1.f, -1.f, -1.f), //
  Vector3(-1.f, 1.f, 1.f),   //
  Vector3(-1.f, 1.f, -1.f)   //
}};

std::array<Vector3, 4> PanoramaToCubeMapTools::FACE_DOWN{
  {Vector3(-1.f, 1.f, -1.f), //
   Vector3(1.f, 1.f, -1.f),  //
   Vector3(-1.f, 1.f, 1.f),  //
   Vector3(1.f, 1.f, 1.f)}};

std::array<Vector3, 4> PanoramaToCubeMapTools::FACE_UP{{
  Vector3(-1.f, -1.f, 1.f),  //
  Vector3(1.f, -1.f, 1.f),   //
  Vector3(-1.f, -1.f, -1.f), //
  Vector3(1.f, -1.f, -1.f)   //
}};

CubeMapInfo PanoramaToCubeMapTools::ConvertPanoramaToCubemap(
  const Float32Array& float32Array, size_t inputWidth, size_t inputHeight,
  size_t size)
{
  CubeMapInfo cubeMapInfo;

  if (float32Array.size() != inputWidth * inputHeight * 3) {
    BABYLON_LOG_ERROR("PanoramaToCubeMapTools",
                      "ConvertPanoramaToCubemap: input size is wrong")
    return cubeMapInfo;
  }

  cubeMapInfo.front = ArrayBufferView(CreateCubemapTexture(size, FACE_FRONT, float32Array,
                                           inputWidth, inputHeight));
  cubeMapInfo.back  = ArrayBufferView(CreateCubemapTexture(size, FACE_BACK, float32Array,
                                          inputWidth, inputHeight));
  cubeMapInfo.left  = ArrayBufferView(CreateCubemapTexture(size, FACE_LEFT, float32Array,
                                          inputWidth, inputHeight));
  cubeMapInfo.right = ArrayBufferView(CreateCubemapTexture(size, FACE_RIGHT, float32Array,
                                           inputWidth, inputHeight));
  cubeMapInfo.up = ArrayBufferView(CreateCubemapTexture(size, FACE_UP, float32Array, inputWidth,
                                        inputHeight));
  cubeMapInfo.down       = ArrayBufferView(CreateCubemapTexture(size, FACE_DOWN, float32Array,
                                          inputWidth, inputHeight));
  cubeMapInfo.size       = size;
  cubeMapInfo.type       = Constants::TEXTURETYPE_FLOAT;
  cubeMapInfo.format     = Constants::TEXTUREFORMAT_RGB;
  cubeMapInfo.gammaSpace = false;

  return cubeMapInfo;
}

Float32Array PanoramaToCubeMapTools::CreateCubemapTexture(
  size_t texSize, const std::array<Vector3, 4>& faceData,
  const Float32Array& float32Array, size_t inputWidth, size_t inputHeight)
{
  Float32Array textureArray(texSize * texSize * 4 * 3);

  auto texSizef = static_cast<float>(texSize);
  auto rotDX1   = faceData[1].subtract(faceData[0]).scale(1.f / texSizef);
  auto rotDX2   = faceData[3].subtract(faceData[2]).scale(1.f / texSizef);

  auto dy = 1.f / static_cast<float>(texSize);
  auto fy = 0.f;

  for (size_t y = 0; y < texSize; ++y) {
    auto xv1 = faceData[0];
    auto xv2 = faceData[2];

    for (size_t x = 0; x < texSize; ++x) {
      auto v = xv2.subtract(xv1).scale(fy).add(xv1);
      v.normalize();

      auto color
        = CalcProjectionSpherical(v, float32Array, inputWidth, inputHeight);

      // 3 channels per pixels
      textureArray[y * texSize * 3 + (x * 3) + 0] = color.r;
      textureArray[y * texSize * 3 + (x * 3) + 1] = color.g;
      textureArray[y * texSize * 3 + (x * 3) + 2] = color.b;

      xv1 = xv1.add(rotDX1);
      xv2 = xv2.add(rotDX2);
    }

    fy += dy;
  }

  return textureArray;
}

Color3 PanoramaToCubeMapTools::CalcProjectionSpherical(
  const Vector3& vDir, const Float32Array& float32Array, size_t inputWidth,
  size_t inputHeight)
{
  auto theta = std::atan2(vDir.z, vDir.x);
  auto phi   = std::acos(vDir.y);

  while (theta < -Math::PI) {
    theta += 2.f * Math::PI;
  }
  while (theta > Math::PI) {
    theta -= 2.f * Math::PI;
  }

  auto dx = theta / Math::PI;
  auto dy = phi / Math::PI;

  // recenter.
  dx = dx * 0.5f + 0.5f;

  auto px = static_cast<int>(std::round(dx * static_cast<float>(inputWidth)));
  if (px < 0) {
    px = 0;
  }
  else if (static_cast<size_t>(px) >= inputWidth) {
    px = static_cast<int>(inputWidth) - 1;
  }

  auto py = static_cast<int>(std::round(dy * static_cast<float>(inputHeight)));
  if (py < 0) {
    py = 0;
  }
  else if (static_cast<size_t>(py) >= inputHeight) {
    py = static_cast<int>(inputHeight) - 1;
  }

  auto inputY = (inputHeight - static_cast<size_t>(py) - 1);
  auto _px    = static_cast<size_t>(px);
  auto r      = float32Array[inputY * inputWidth * 3 + (_px * 3) + 0];
  auto g      = float32Array[inputY * inputWidth * 3 + (_px * 3) + 1];
  auto b      = float32Array[inputY * inputWidth * 3 + (_px * 3) + 2];

  return Color3(r, g, b);
}

} // end of namespace BABYLON
