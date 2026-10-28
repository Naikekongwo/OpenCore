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
namespace OpenUI
{

class UIElement : public IDrawableObject
{
  public:
    /**
     * @brief 构造一个 UIElement 对象。
     * @param id      对象的唯一标识符。
     * @param layer   渲染图层（值越小越靠后）。
     * @param texture 关联的纹理智能指针（可为空，例如纯容器控件）。
     */
    UIElement(std::string_view id, short layer, shared_ptr<Texture> texture);
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

#pragma region 背景样式
    void setBackgroundColor(Color color);
    Color getBackgroundColor() const { return m_backgroundColor; }

    /**
     * @brief 设置圆角半径（相对逻辑画面高度，0 表示方角）
     * @note 只有 ImageBoard 会把圆角用于裁剪自身内容，其余控件只作用于背景。
     */
    void  setCornerRadius(float heightRatio);
    float getCornerRadius() const { return m_cornerRadius; }

    void setBorderColor(Color color);
    Color getBorderColor() const { return m_borderColor; }

    /**
     * @brief 设置描边宽度（相对逻辑画面高度，0 表示无描边）
     */
    void  setBorderWidth(float heightRatio);
    float getBorderWidth() const { return m_borderWidth; }
#pragma endregion

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

    Color m_backgroundColor = None; ///< 背景填充色
    float m_cornerRadius    = 0.0f; ///< 圆角半径（相对逻辑画面高度）
    Color m_borderColor     = None; ///< 描边颜色
    float m_borderWidth     = 0.0f; ///< 描边宽度（相对逻辑画面高度）
    bool  m_textureDirty    = true; ///< 纹理缓存脏污标志

    shared_ptr<Texture> m_textureCache; ///< 离屏纹理缓存

    /// 圆角半径换算成逻辑像素
    float cornerRadiusPixels() const;
    /// 描边宽度换算成逻辑像素
    float borderWidthPixels() const;
    /// 是否配置了可见描边
    bool hasVisibleBorder() const;

  private:
    void drawBackground();
    void ensureBackgroundCache(const SDL_Rect &bounds, float radiusPx,
                               float borderPx);

    shared_ptr<Texture> m_backgroundCache; ///< 圆角背景缓存
    bool m_backgroundDirty = true;         ///< 背景缓存脏污标志
};

} // namespace OpenUI

#endif //_UIELEMENT_H_