#include <babylon/gamepads/gamepad_manager.h>

namespace BABYLON {

GamepadManager::GamepadManager(Scene* scene) : _scene{scene}
{
}

GamepadManager::~GamepadManager() = default;

std::vector<std::shared_ptr<Gamepad>>& GamepadManager::gamepads()
{
  return _babylonGamepads;
}

Gamepad* GamepadManager::getGamepadByType(unsigned int /*type*/)
{
  return nullptr;
}

void GamepadManager::dispose()
{
}

Gamepad* GamepadManager::_addNewGamepad(Gamepad* gamepad)
{
  return gamepad;
}

void GamepadManager::_startMonitoringGamepads()
{
}

void GamepadManager::_stopMonitoringGamepads()
{
}

void GamepadManager::_checkGamepadsStatus()
{
}

void GamepadManager::_updateGamepadObjects()
{
}

} // end of namespace BABYLON
