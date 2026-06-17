#pragma once

#include "GraphicsInfo.hpp"
#include "ResourceInfo.hpp"
#include <memory>
#include <string>

using std::string;
using std::unique_ptr;

class Stage;

/**
 * @brief 游戏信息注册表
 *
 */
struct GameInfo
{
    string gameName = "openengine-game";

    uint8_t version_major = 1;
    uint8_t version_minor = 0;
    bool    beta          = false;

    GraphicsInfo _graphicsInfo;
    ResourceInfo _resourceInfo;

    unique_ptr<Stage> entranceStage;
};