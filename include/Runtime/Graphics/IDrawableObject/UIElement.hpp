/**
 * @file UIElement.hpp
 * @brief UI 控件的基类定义。
 *
 * 继承自 IDrawableObject，为所有 UI 元素（按钮、标签、面板等）提供统一的
 * 碰撞箱计算、纹理生成接口和生命周期管理。
 */

#ifndef _UIELEMENT_H_
#define _UIELEMENT_H_

#include "Core/Math/OpenCore_Color.hpp"
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
    UIElement(const string &id, short layer, shared_ptr<Texture> texture);
    ~UIElement() override;

    void onUpdate(float totalTime) override;
    void parseEvents(Event *event, float totalTime) override;
    void Draw() override;
    bool onDestroy() override;

    /**
     * @brief 生成或更新 UI 元素的底层 SDL 纹理。
     *        派生类应直接使用 m_textureCache 或 this->texture 进行渲染。
     * @return true  表示生成成功；
     * @return false 表示生成失败（派生类可按需实现）。
     * @note 基类默认返回 false，需要具体控件覆盖实现。
     */
    virtual bool     generateTexture() { return false; }
    virtual SDL_Rect getLogicalBounds() override;
    virtual SDL_Rect getPhysicalBounds() override;

    void setBackgroundColor(Color color) { this->color = color; }

  protected:
    /**
     * @enum InteractionState
     * @brief UI 元素的交互状态枚举。
     */
    enum class InteractionState
    {
        Normal,  ///< 无交互
        Hovered, ///< 鼠标悬停
        Pressed  ///< 鼠标按下
    };

    InteractionState m_interactionState =
        InteractionState::Normal; ///< 当前交互状态

    /**
     * @brief 鼠标首次进入元素区域时调用。
     * @param event    当前事件对象。
     * @param mousePos 鼠标在窗口中的坐标。
     */
    virtual void onMouseEnter(Event *event, const SDL_Point &mousePos) {}

    /**
     * @brief 鼠标离开元素区域时调用（含按下拖出场景）。
     * @param event    当前事件对象。
     * @param mousePos 鼠标在窗口中的坐标。
     */
    virtual void onMouseExit(Event *event, const SDL_Point &mousePos) {}

    /**
     * @brief 鼠标在元素区域内按下时调用。
     * @param event    当前事件对象。
     * @param mousePos 鼠标在窗口中的坐标。
     */
    virtual void onMousePress(Event *event, const SDL_Point &mousePos) {}

    /**
     * @brief 鼠标在元素区域内释放时调用（不论是否触发点击）。
     * @param event    当前事件对象。
     * @param mousePos 鼠标在窗口中的坐标。
     */
    virtual void onMouseRelease(Event *event, const SDL_Point &mousePos) {}

    /**
     * @brief 一次完整的点击（在元素内按下并释放）时调用。
     * @param event    当前事件对象。
     * @param mousePos 鼠标在窗口中的坐标。
     */
    virtual void onClick(Event *event, const SDL_Point &mousePos) {}

    Color color          = None; /// <控件的背景色: 默认为透明>
    bool  m_textureDirty = true; /// <纹理缓存脏污标志>

    shared_ptr<Texture> m_textureCache; ///< 离屏纹理缓存
};

#endif //_UIELEMENT_H_