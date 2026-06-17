/**
 * @file FrameCounter.hpp
 * @brief 帧率计数器 UI 控件。
 *
 * 用于实时计算并显示当前渲染帧率（FPS），通常作为调试或性能监控工具。
 */

#pragma once

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include <SDL3_ttf/SDL_ttf.h>

/**
 * @class FrameCounter
 * @brief 帧率显示器。
 *
 * 继承自 UIElement，在每一帧更新时计算时间差，得出当前 FPS，
 * 并通过 PackageManager 加载字体将 FPS 数值渲染为文本。
 */
class FrameCounter : public UIElement
{
  public:
    /**
     * @brief 构造帧率计数器对象。
     * @param id      唯一标识符。
     * @param layer   渲染图层。
     */
    FrameCounter(const std::string &id, uint8_t layer);
    void parseEvents(Event *event, float totalTime) override;
    void Draw() override;
    void onUpdate(float totalTime) override;

    /// 设置字体名称（对应 PackageManager 中注册的资源名）
    void setFontName(const std::string &name)
    {
        fontName = name;
        font.reset();
    }
    /// 设置字体大小（磅值）
    void setFontSize(int size)
    {
        fontSize = size;
        font.reset();
    }

  private:
    float                lastTime = 0.0f;       ///< 上一帧的时间戳
    float                FPS      = 0.0f;       ///< 当前计算的帧率
    std::string          fontName = "Font_Eng"; ///< 字体资源名称
    int                  fontSize = 16;         ///< 字体大小
    shared_ptr<TTF_Font> font;                  ///< 缓存的字体指针
};