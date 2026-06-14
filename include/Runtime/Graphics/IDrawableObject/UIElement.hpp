/**
 * @file UIElement.hpp
 * @brief UI 控件的基类定义。
 *
 * 继承自 IDrawableObject，为所有 UI 元素（按钮、标签、面板等）提供统一的
 * 碰撞箱计算、纹理生成接口和生命周期管理。
 */

#ifndef _UIELEMENT_H_
#define _UIELEMENT_H_

#include "IDrawableObject.hpp"

/**
 * @class UIElement
 * @brief UI 元素的抽象基类。
 *
 * 所有 UI 控件都应直接或间接继承此类。它基于 IDrawableObject 提供的视觉状态
 * （位置、缩放、锚点）实现了标准的逻辑/物理碰撞箱计算，并声明了纹理生成接口。
 */
class UIElement : public IDrawableObject
{
  public:
    /**
     * @brief 构造一个 UIElement 对象。
     * @param id      对象的唯一标识符。
     * @param layer   渲染图层（值越小越靠后）。
     * @param texture 关联的纹理智能指针（可为空，例如纯容器控件）。
     */
    UIElement(const string &id, short layer, unique_ptr<Texture> texture);
    ~UIElement() override;

    virtual SDL_Rect getLogicalBounds() override;
    virtual SDL_Rect getPhysicalBounds() override;

    void setBackgroundColor(SDL_Color color) { this->color = color; }

    void onUpdate(float totalTime) override;
    void parseEvents(Event *event, float totalTime) override;
    void Draw() override;

    /**
     * @brief 生成或更新 UI 元素的底层 SDL 纹理。
     * @param texture 指向 SDL_Texture 的指针，通常由渲染器创建。
     * @return true  表示生成成功；
     * @return false 表示生成失败（派生类可按需实现）。
     * @note 基类默认返回 false，需要具体控件覆盖实现。
     */
    virtual bool generateTexture(SDL_Texture *texture) { return false; }
    bool onDestroy() override;

  protected:
    bool m_textureDirty =
        true; ///< 纹理缓存脏污标志，窗口缩放或内容变更时置 true
    SDL_Texture *m_textureCache =
        nullptr; ///< 离屏纹理缓存，由基类统一管理生命周期

    SDL_Color color = {0, 0, 0, 0};
};

#endif //_UIELEMENT_H_