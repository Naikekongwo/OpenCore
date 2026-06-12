#include "Core/Event/ControllerManager.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <shared_mutex>

ControllerManager &ControllerManager::GetInstance()
{
    static ControllerManager instance;
    return instance;
}

void ControllerManager::Init()
{
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    int numJoysticks;
    SDL_JoystickID *joysticks = SDL_GetJoysticks(&numJoysticks);
    for (int i = 0; i < numJoysticks; ++i)
    {
        if (SDL_IsGamepad(joysticks[i]))
        {
            SDL_Gamepad *controller = SDL_OpenGamepad(joysticks[i]);
            if (controller)
            {
                SDL_JoystickID id = SDL_GetJoystickID(
                    SDL_GetGamepadJoystick(controller));
                int playerIndex = FindFreePlayerIndex();
                ControllerInfo info;
                info.controller = controller;
                info.instanceId = id;
                std::memset(info.buttonState, 0, sizeof(info.buttonState));
                std::memset(info.axisState, 0, sizeof(info.axisState));
                m_players[playerIndex] = info;
                m_instanceToPlayer[id] = playerIndex;
            }
        }
    }
}

void ControllerManager::Shutdown()
{
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    for (auto &pair : m_players)
    {
        if (pair.second.controller)
        {
            SDL_CloseGamepad(pair.second.controller);
        }
    }
    m_players.clear();
    m_instanceToPlayer.clear();
}

void ControllerManager::HandleEvent(const SDL_Event &event)
{
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    switch (event.type)
    {
    case SDL_EVENT_GAMEPAD_ADDED:
    {
        SDL_JoystickID instanceId = event.gdevice.which;
        SDL_Gamepad *controller = SDL_OpenGamepad(instanceId);
        if (controller)
        {
            SDL_JoystickID id = SDL_GetJoystickID(
                SDL_GetGamepadJoystick(controller));
            int playerIndex = FindFreePlayerIndex();
            ControllerInfo info;
            info.controller = controller;
            info.instanceId = id;
            std::memset(info.buttonState, 0, sizeof(info.buttonState));
            std::memset(info.axisState, 0, sizeof(info.axisState));
            m_players[playerIndex] = info;
            m_instanceToPlayer[id] = playerIndex;
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_REMOVED:
    {
        SDL_JoystickID id = event.gdevice.which;
        auto it = m_instanceToPlayer.find(id);
        if (it != m_instanceToPlayer.end())
        {
            int player = it->second;
            auto playerIt = m_players.find(player);
            if (playerIt != m_players.end())
            {
                SDL_CloseGamepad(playerIt->second.controller);
                m_players.erase(playerIt);
            }
            m_instanceToPlayer.erase(it);
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    {
        SDL_JoystickID id = event.gbutton.which;
        auto it = m_instanceToPlayer.find(id);
        if (it != m_instanceToPlayer.end())
        {
            int player = it->second;
            auto &info = m_players[player];
            if (event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT)
            {
                info.buttonState[event.gbutton.button] =
                    (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) ? 1 : 0;
            }
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    {
        SDL_JoystickID id = event.gaxis.which;
        auto it = m_instanceToPlayer.find(id);
        if (it != m_instanceToPlayer.end())
        {
            int player = it->second;
            auto &info = m_players[player];
            if (event.gaxis.axis < SDL_GAMEPAD_AXIS_COUNT)
            {
                info.axisState[event.gaxis.axis] = event.gaxis.value;
            }
        }
        break;
    }
    }
}

bool ControllerManager::IsButtonPressed(int playerIndex,
                                        SDL_GamepadButton button) const
{
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = m_players.find(playerIndex);
    if (it != m_players.end() && button < SDL_GAMEPAD_BUTTON_COUNT)
    {
        return it->second.buttonState[button] != 0;
    }
    return false;
}

Uint8 ControllerManager::GetButton(int playerIndex,
                                   SDL_GamepadButton button) const
{
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = m_players.find(playerIndex);
    if (it != m_players.end() && button < SDL_GAMEPAD_BUTTON_COUNT)
    {
        return it->second.buttonState[button];
    }
    return 0;
}

Sint16 ControllerManager::GetAxis(int playerIndex,
                                  SDL_GamepadAxis axis) const
{
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = m_players.find(playerIndex);
    if (it != m_players.end() && axis < SDL_GAMEPAD_AXIS_COUNT)
    {
        return it->second.axisState[axis];
    }
    return 0;
}

SDL_Gamepad *
ControllerManager::GetControllerForPlayer(int playerIndex) const
{
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = m_players.find(playerIndex);
    return (it != m_players.end()) ? it->second.controller : nullptr;
}

int ControllerManager::GetPlayerIndexFromInstanceID(
    SDL_JoystickID instanceId) const
{
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = m_instanceToPlayer.find(instanceId);
    return (it != m_instanceToPlayer.end()) ? it->second : -1;
}

void ControllerManager::RumblePlayer(int playerIndex, Uint16 lowFreq,
                                     Uint16 highFreq, Uint32 durationMs)
{
    // 直接在调用线程（主线程）执行震动，SDL_RumbleGamepad 是轻量非阻塞的
    SDL_Gamepad *controller = GetControllerForPlayer(playerIndex);
    if (controller)
    {
        SDL_PropertiesID props = SDL_GetGamepadProperties(controller);
        bool hasRumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
        if (hasRumble)
        {
            SDL_RumbleGamepad(controller, lowFreq, highFreq, durationMs);
        }
    }
}

size_t ControllerManager::GetConnectedPlayerCount() const
{
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    return m_players.size();
}

int ControllerManager::FindFreePlayerIndex() const
{
    // 必须在已持有锁的情况下调用（外部保证）
    int index = 0;
    while (m_players.find(index) != m_players.end())
    {
        ++index;
    }
    return index;
}