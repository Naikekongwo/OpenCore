/**
 * @file ControllerManager.hpp
 * @author your name (you@domain.com)
 * @brief 手柄管理器
 * @version 0.1
 * @date 2026-04-23
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include <SDL3/SDL.h>
#include <shared_mutex>
#include <unordered_map>

class ControllerManager
{
  public:
    static ControllerManager &GetInstance();

    void Init();
    void Shutdown();

    // 由主循环调用，处理手柄事件
    void HandleEvent(const SDL_Event &event);

    // 玩家输入查询（玩家索引从 0 开始，若玩家未连接返回默认值）
    bool IsButtonPressed(int playerIndex,
                         SDL_GamepadButton button) const;
    Uint8 GetButton(int playerIndex, SDL_GamepadButton button) const;
    Sint16 GetAxis(int playerIndex, SDL_GamepadAxis axis) const;

    // 获取玩家对应的控制器指针（用于高级操作，如震动）
    SDL_Gamepad *GetControllerForPlayer(int playerIndex) const;

    // 根据设备实例 ID 获取玩家索引，未找到返回 -1
    int GetPlayerIndexFromInstanceID(SDL_JoystickID instanceId) const;

    // 在主线程同步触发手柄震动（非阻塞）
    void RumblePlayer(int playerIndex, Uint16 lowFreq, Uint16 highFreq,
                      Uint32 durationMs);

    // 获取当前已连接的玩家数量
    size_t GetConnectedPlayerCount() const;

  private:
    ControllerManager() = default;
    ~ControllerManager() = default;
    ControllerManager(const ControllerManager &) = delete;
    ControllerManager &operator=(const ControllerManager &) = delete;

    struct ControllerInfo
    {
        SDL_Gamepad *controller;
        SDL_JoystickID instanceId;
        Uint8 buttonState[SDL_GAMEPAD_BUTTON_COUNT];
        Sint16 axisState[SDL_GAMEPAD_AXIS_COUNT];
    };

    mutable std::shared_mutex rw_mutex_; // 保护 m_players 和 m_instanceToPlayer

    std::unordered_map<int, ControllerInfo> m_players; // 玩家索引 -> 控制器信息
    std::unordered_map<SDL_JoystickID, int>
        m_instanceToPlayer; // 实例 ID -> 玩家索引

    int FindFreePlayerIndex() const; // 需在锁保护下调用
};