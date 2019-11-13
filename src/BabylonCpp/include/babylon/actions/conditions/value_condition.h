#ifndef BABYLON_ACTIONS_CONDITIONS_VALUE_CONDITION_H
#define BABYLON_ACTIONS_CONDITIONS_VALUE_CONDITION_H

#include <babylon/actions/condition.h>
#include <babylon/babylon_api.h>

namespace BABYLON {

class AnimationValue;

/**
 * @brief Defines specific conditional operators as extensions of Condition.
 */
class BABYLON_SHARED_EXPORT ValueCondition : public Condition {

private:
  /** Statics **/

  /**
   * Internal only
   */
  static constexpr unsigned int _IsEqual = 0;

  /**
   * Internal only
   */
  static constexpr unsigned int _IsDifferent = 1;

  /**
   * Internal only
   */
  static constexpr unsigned int _IsGreater = 2;

  /**
   * Internal only
   */
  static constexpr unsigned int _IsLesser = 3;

public:
  /** Statics **/

  /**
   * @brief Returns the number for IsEqual.
   */
  static constexpr unsigned int IsEqual()
  {
    return ValueCondition::_IsEqual;
  }

  /**
   * @brief Returns the number for IsDifferent.
   */
  static constexpr unsigned int IsDifferent()
  {
    return ValueCondition::_IsDifferent;
  }

  /**
   * @brief Returns the number for IsGreater.
   */
  static constexpr unsigned int IsGreater()
  {
    return ValueCondition::_IsGreater;
  }

  /**
   * @brief Returns the number for IsLesser.
   */
  static constexpr unsigned int IsLesser()
  {
    return ValueCondition::_IsLesser;
  }

public:
  /**
   * @brief Creates a new ValueCondition.
   * @param actionManager manager for the action the condition applies to
   * @param target for the action
   * @param propertyPath path to specify the property of the target the
   * conditional operator uses
   * @param value the value compared by the conditional operator against the
   * current value of the property
   * @param operator the conditional operator, default ValueCondition.IsEqual
   */
  ValueCondition(ActionManager* actionManager, const IAnimatablePtr& target,
                 const std::string& propertyPath, AnimationValue* value,
                 unsigned int operatorType = ValueCondition::IsEqual());
  virtual ~ValueCondition(); // = default

  /** Methods **/

  /**
   * @brief Compares the given value with the property value for the specified
   * conditional operator.
   * @returns the result of the comparison
   */
  bool isValid() override;

  /**
   * @brief Serialize the ValueCondition into a JSON compatible object.
   * @returns serialization object
   */
  json serialize() const override;

  /** Statics **/

  /**
   * @brief Gets the name of the conditional operator for the ValueCondition.
   * @param operator the conditional operator
   * @returns the name
   */
  static std::string GetOperatorName(unsigned int operatorType);

private:
  std::string _propertyPath;
  unsigned int _operatorType;

  /**
   * Internal only
   */
  IAnimatablePtr _target;

  /**
   * Internal only
   */
  IAnimatablePtr _effectiveTarget;

  /**
   * Internal only
   */
  std::string _property;

  AnimationValue* _value;

}; // end of class ValueCondition

} // end of namespace BABYLON

#endif // end of BABYLON_ACTIONS_CONDITIONS_VALUE_CONDITION_H
