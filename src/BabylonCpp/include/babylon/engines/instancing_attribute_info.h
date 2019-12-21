#ifndef BABYLON_ENGINES_INSTANCING_ATTRIBUTE_INFO_H
#define BABYLON_ENGINES_INSTANCING_ATTRIBUTE_INFO_H

#include <babylon/babylon_api.h>
#include <babylon/interfaces/igl_rendering_context.h>

namespace BABYLON {

/**
 * @brief Interface for attribute information associated with buffer instanciation.
 */
struct BABYLON_SHARED_EXPORT InstancingAttributeInfo {
  /**
   * Index/offset of the attribute in the vertex shader
   */
  unsigned int index;

  /**
   * size of the attribute, 1, 2, 3 or 4
   */
  int attributeSize;

  /**
   * type of the attribute, gl.BYTE, gl.UNSIGNED_BYTE, gl.SHORT, gl.UNSIGNED_SHORT, gl.FIXED,
   * gl.FLOAT. default is FLOAT
   */
  unsigned int attribyteType = GL::FLOAT;

  /**
   * normalization of fixed-point data. behavior unclear, use FALSE, default is FALSE
   */
  bool normalized = false;

  /**
   * Offset of the data in the Vertex Buffer acting as the instancing buffer
   */
  int offset;

  /**
   * Name of the GLSL attribute, for debugging purpose only
   */
  std::string attributeName;
}; // end of struct InstancingAttributeInfo

} // end of namespace BABYLON

#endif // end of BABYLON_ENGINES_INSTANCING_ATTRIBUTE_INFO_H
