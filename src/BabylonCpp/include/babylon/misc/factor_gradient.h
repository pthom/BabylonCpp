#ifndef BABYLON_MISC_FACTOR_GRADIENT_H
#define BABYLON_MISC_FACTOR_GRADIENT_H

#include <optional>

#include <babylon/babylon_api.h>
#include <babylon/misc/ivalue_gradient.h>

namespace BABYLON {

/**
 * @brief Class used to store factor gradient.
 */
struct BABYLON_SHARED_EXPORT FactorGradient : public IValueGradient {

  FactorGradient();
  ~FactorGradient(); // = default

  /**
   * Gets or sets first associated factor
   */
  float factor1;

  /**
   * Gets or sets second associated factor
   */
  std::optional<float> factor2;

  /**
   * @brief Will get a number picked randomly between factor1 and factor2.
   * If factor2 is undefined then factor1 will be used
   * @returns the picked number
   */
  float getFactor() const;

}; // end of struct FactorGradient

bool operator==(const FactorGradient& lhs, const FactorGradient& rhs);
bool operator!=(const FactorGradient& lhs, const FactorGradient& rhs);

} // end of namespace BABYLON

#endif // end of BABYLON_MISC_FACTOR_GRADIENT_H
