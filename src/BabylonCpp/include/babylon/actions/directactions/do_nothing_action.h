#ifndef BABYLON_ACTIONS_DIRECTACTIONS_DO_NOTHING_ACTION_H
#define BABYLON_ACTIONS_DIRECTACTIONS_DO_NOTHING_ACTION_H

#include <babylon/actions/action.h>
#include <babylon/babylon_api.h>
#include <babylon/engines/constants.h>

namespace BABYLON {

/**
 * @brief This defines an action responsible that does nothing once triggered.
 * @see http://doc.babylonjs.com/how_to/how_to_use_actions
 */
class BABYLON_SHARED_EXPORT DoNothingAction : public Action {

public:
  /**
   * Instantiate the action
   * @param triggerOptions defines the trigger options
   * @param condition defines the trigger related conditions
   */
  DoNothingAction(unsigned int triggerOptions
                  = Constants::ACTION_NothingTrigger,
                  Condition* condition = nullptr);
  ~DoNothingAction(); // = default

  /**
   * @brief Execute the action and do nothing.
   */
  void execute(const std::optional<IActionEvent>& evt) override;

  /**
   * @brief Serializes the actions and its related information.
   * @param parent defines the object to serialize in
   * @returns the serialized object
   */
  json serialize(json& parent) const override;

}; // end of class DoNothingAction

} // end of namespace BABYLON

#endif // end of BABYLON_ACTIONS_DIRECTACTIONS_DO_NOTHING_ACTION_H
