#pragma once

/**
 * @brief 游戏图形信息注册表
 *
 */
struct GraphicsInfo
{
    int  resolutionWidth  = 1920;
    int  resolutionHeight = 1080;
    int  targetFrameRate  = 144;
    bool nearestScaling   = false;
    bool keepRatio        = false;
    bool resizable        = true;
};