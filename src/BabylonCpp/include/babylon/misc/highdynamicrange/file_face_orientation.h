#ifndef BABYLON_MISC_HIGH_DYNAMIC_RANGE_FILE_FACE_ORIENTATION_H
#define BABYLON_MISC_HIGH_DYNAMIC_RANGE_FILE_FACE_ORIENTATION_H

#include <babylon/babylon_api.h>
#include <babylon/math/vector3.h>

namespace BABYLON {

struct BABYLON_SHARED_EXPORT FileFaceOrientation {

  FileFaceOrientation(const std::string& name,
                      const Vector3& worldAxisForNormal,
                      const Vector3& worldAxisForFileX,
                      const Vector3& worldAxisForFileY);
  FileFaceOrientation(const FileFaceOrientation& other);
  FileFaceOrientation(FileFaceOrientation&& other);
  FileFaceOrientation& operator=(const FileFaceOrientation& other);
  FileFaceOrientation& operator=(FileFaceOrientation&& other);
  ~FileFaceOrientation(); // = default

  std::string name;
  // The world axis corresponding to the normal to the face
  Vector3 worldAxisForNormal;
  // The world axis corresponding to texture right x-axis in file
  Vector3 worldAxisForFileX;
  // The world axis corresponding to texture down y-axis in file
  Vector3 worldAxisForFileY;

}; // end of struct FileFaceOrientation

} // end of namespace BABYLON

#endif // end of BABYLON_MISC_HIGH_DYNAMIC_RANGE_FILE_FACE_ORIENTATION_H
