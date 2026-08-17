/**
 * @file GraphicsManager.hpp
 * @author Naikekongwo
 * @brief
 * @version 0.1
 * @date 2026-03-08
 *
 * @copyright OpenGames Studio Copyright (c) 2026
 *
 */

#ifndef _GFXCORE_H_
#define _GFXCORE_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_Log.h>

#include "Core/Math/OpenCore_Color.hpp"
#include "Core/Math/OpenCore_Rect.hpp"
#include <cmath>
#include <string>

#include <cstdarg>
#include <memory>

struct Texture;

/**
 * @brief 图形模块
 * @todo 急需加入视窗裁切的相关方法
 */
class GraphicsManager
{
  public:
    /**
     * @brief 获取图形管理器的实例
     *
     * @return GraphicsManager&
     */
    static GraphicsManager &getInstance();

    /**
     * @brief 初始化图形管理器
     *
     * @return true
     * @return false
     */
    bool Init();

    /**
     * @brief 刷新窗口信息
     *
     */
    void refreshWindowProperties();

    /**
     * @brief 清理回收相关数据
     *
     */
    void CleanUp();

    /**
     * @brief 获取SDL的窗口句柄
     *
     * @return SDL_Window*
     */
    SDL_Window *getWindow() const { return window; }

    /**
     * @brief 获取SDL的渲染器句柄
     *
     * @return SDL_Renderer*
     */
    SDL_Renderer *getRenderer() const { return renderer; }

    /**
     * @brief 设置渲染目标
     * @details 当texture为nullptr时候渲染到屏幕上，否则就是在纹理上绘制
     * @param texture
     * @return int
     */
    int setRenderTarget(SDL_Texture *texture);

    [[deprecated("use createTextureShared instead")]]
    SDL_Texture *createTexture(uint16_t w, uint16_t h);

    std::shared_ptr<SDL_Texture> createTextureShared(uint16_t w, uint16_t h);

    Rect getSccissorRect() const
    {
        Rect window{0, 0, 0, 0};
        window.w = TargetWindowWidth;
        window.h = TargetWindowHeight;
        return window;
    }

    /// 设计基准宽度（1920×1080 基准的宽度方向），相对字号换算基准
    static constexpr int kDesignWidth = 1920;

    /**
     * @brief 设计基准字号 → 当前逻辑分辨率字号（按宽度方向缩放）。
     * @param designSize 1920 设计基准宽度下所见字号
     * @return 换算后的逻辑像素字号
     */
    short designFontSize(short designSize) const noexcept
    {
        return static_cast<short>(std::round(static_cast<float>(designSize) *
                                             TargetWindowWidth / kDesignWidth));
    }

    /**
     * @brief 用指定颜色填充一个矩形区域
     * @param rect  目标矩形（逻辑坐标）
     * @param color 填充颜色（含 Alpha）
     */
    void FillRect(const Rect &rect, const Color &color);

    /**
     * @brief 截取当前渲染器画面，返回 shared_ptr<Texture>（网格 1×1）。
     *        可直接用于 changeTexture / ImageBoard 等。
     * @return shared_ptr<Texture>，失败返回 nullptr
     */
    std::shared_ptr<Texture> captureScreen();

  private:
    SDL_Window   *window;
    SDL_Renderer *renderer;

    int TargetWindowHeight = 1080;
    int TargetWindowWidth  = 1920;
};

#endif //_GFXCORE_H_