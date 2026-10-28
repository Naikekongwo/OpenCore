
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include "Runtime/Graphics/UI/RoundedRect.hpp"

#include <algorithm>

namespace OpenUI
{

namespace
{

bool SameColor(const Color &a, const Color &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

} // namespace

SDL_Rect UIElement::getLogicalBounds()
{
    if (!VState)
    {
        LOG("VState 为空，无法获取逻辑碰撞箱");
        return SDL_Rect{0, 0, 0, 0};
    }

    const auto &state = *VState;

    float logicalWidth  = absWidth * state.scale[0];
    float logicalHeight = absHeight * state.scale[1];

    float logicalX = state.Position[0];
    float logicalY = state.Position[1];

    switch (state.Anchor)
    {
    case AnchorPoint::TopLeft:
        break;
    case AnchorPoint::TopCenter:
        logicalX -= logicalWidth * 0.5f;
        break;
    case AnchorPoint::TopRight:
        logicalX -= logicalWidth;
        break;
    case AnchorPoint::MiddleLeft:
        logicalY -= logicalHeight * 0.5f;
        break;
    case AnchorPoint::Center:
        logicalX -= logicalWidth * 0.5f;
        logicalY -= logicalHeight * 0.5f;
        break;
    case AnchorPoint::MiddleRight:
        logicalX -= logicalWidth;
        logicalY -= logicalHeight * 0.5f;
        break;
    case AnchorPoint::BottomLeft:
        logicalY -= logicalHeight;
        break;
    case AnchorPoint::BottomCenter:
        logicalX -= logicalWidth * 0.5f;
        logicalY -= logicalHeight;
        break;
    case AnchorPoint::BottomRight:
        logicalX -= logicalWidth;
        logicalY -= logicalHeight;
        break;
    }

    return SDL_Rect{static_cast<int>(std::round(logicalX)),
                    static_cast<int>(std::round(logicalY)),
                    static_cast<int>(std::round(logicalWidth)),
                    static_cast<int>(std::round(logicalHeight))};
}

SDL_Rect UIElement::getPhysicalBounds() { return getLogicalBounds(); }

UIElement::~UIElement()
{
    m_textureCache.reset();
    m_backgroundCache.reset();
}

bool UIElement::onDestroy()
{
    m_textureCache.reset();
    m_backgroundCache.reset();
    IDrawableObject::onDestroy();
    return true;
}

void UIElement::onUpdate(float totalTime)
{
    if (!isAnimeFinished())
    {
        IDrawableObject::onUpdate(totalTime);
    }

    if (m_textureDirty)
    {
        m_textureCache.reset();
        SDL_Rect bounds = getLogicalBounds();
        m_textureCache  = std::make_shared<Texture>(
            static_cast<uint16_t>(bounds.w), static_cast<uint16_t>(bounds.h),
            size_t(1), size_t(1));
        if (m_textureCache && m_textureCache->get())
            generateTexture();
        m_textureDirty = false;
    }
}

void UIElement::parseEvents(Event *event, float totalTime)
{
    const SDL_Event &sdlEvent = event->GetSDLEvent();

    // 窗口缩放 — 标记所有派生类的纹理缓存为脏污
    if (sdlEvent.type == SDL_EVENT_WINDOW_RESIZED)
    {
        m_textureDirty = true;
        return;
    }

    // ── 通用鼠标交互处理 ──
    // 为所有派生类提供基础的鼠标命中检测与状态机（Normal → Hovered →
    // Pressed）， 通过虚回调暴露扩展点，派生类无需重复实现事件解析逻辑。

    SDL_Point mousePos{};
    SDL_Rect  bounds = getPhysicalBounds();

    switch (sdlEvent.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
    {
        mousePos = {static_cast<int>(sdlEvent.motion.x),
                    static_cast<int>(sdlEvent.motion.y)};

        if (SDL_PointInRect(&mousePos, &bounds))
        {
            if (m_interactionState == InteractionState::Normal)
            {
                m_interactionState = InteractionState::Hovered;
                onMouseEnter(event, mousePos);
            }
            // Pressed 状态下鼠标移回区域内 — 保持 Pressed 不变
        }
        else
        {
            auto oldState      = m_interactionState;
            m_interactionState = InteractionState::Normal;
            if (oldState == InteractionState::Hovered ||
                oldState == InteractionState::Pressed)
            {
                onMouseExit(event, mousePos);
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        if (sdlEvent.button.button == SDL_BUTTON_LEFT)
        {
            mousePos = {static_cast<int>(sdlEvent.button.x),
                        static_cast<int>(sdlEvent.button.y)};

            if (SDL_PointInRect(&mousePos, &bounds))
            {
                m_interactionState = InteractionState::Pressed;
                onMousePress(event, mousePos);
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        if (sdlEvent.button.button == SDL_BUTTON_LEFT)
        {
            mousePos = {static_cast<int>(sdlEvent.button.x),
                        static_cast<int>(sdlEvent.button.y)};

            bool wasPressed = (m_interactionState == InteractionState::Pressed);
            bool inside     = SDL_PointInRect(&mousePos, &bounds);

            if (wasPressed)
            {
                m_interactionState = inside ? InteractionState::Hovered
                                            : InteractionState::Normal;
                onMouseRelease(event, mousePos);

                if (inside)
                {
                    onClick(event, mousePos);
                }
            }
            else
            {
                m_interactionState = inside ? InteractionState::Hovered
                                            : InteractionState::Normal;
            }
        }
        break;
    }

    default:
        break;
    }
}

UIElement::UIElement(std::string_view id, short layer,
                     shared_ptr<Texture> texture)
{
    this->id    = id;
    this->layer = layer;

    if (!texture)
    {
        LOG("元素 {} 使用空纹理初始化（部分容器类元素允许）",
            std::string(id).c_str());
    }
    else
    {
        this->texture = texture;
    }
}

void UIElement::Draw() { drawBackground(); }

void UIElement::setBackgroundColor(Color value)
{
    if (SameColor(m_backgroundColor, value))
        return;

    m_backgroundColor = value;
    m_backgroundDirty = true;
}

void UIElement::setCornerRadius(float heightRatio)
{
    const float clamped = std::clamp(heightRatio, 0.0f, 0.5f);
    if (m_cornerRadius == clamped)
        return;

    m_cornerRadius    = clamped;
    m_backgroundDirty = true;
}

void UIElement::setBorderColor(Color value)
{
    if (SameColor(m_borderColor, value))
        return;

    m_borderColor     = value;
    m_backgroundDirty = true;
}

void UIElement::setBorderWidth(float heightRatio)
{
    const float clamped = std::clamp(heightRatio, 0.0f, 0.5f);
    if (m_borderWidth == clamped)
        return;

    m_borderWidth     = clamped;
    m_backgroundDirty = true;
}

float UIElement::cornerRadiusPixels() const
{
    const float logicalHeight =
        static_cast<float>(OpenCoreManagers::GFXManager.getSccissorRect().h);
    return m_cornerRadius * logicalHeight;
}

float UIElement::borderWidthPixels() const
{
    const float logicalHeight =
        static_cast<float>(OpenCoreManagers::GFXManager.getSccissorRect().h);
    return m_borderWidth * logicalHeight;
}

bool UIElement::hasVisibleBorder() const
{
    return m_borderWidth > 0.0f && m_borderColor.a > 0.001f;
}

void UIElement::drawBackground()
{
    const bool hasBorder = hasVisibleBorder();
    const bool hasFill   = m_backgroundColor.a > 0.001f;
    if (!hasFill && !hasBorder)
        return;

    const SDL_Rect bounds = getLogicalBounds();
    if (bounds.w <= 0 || bounds.h <= 0)
        return;

    const float radiusPx = cornerRadiusPixels();
    const float borderPx = hasBorder ? borderWidthPixels() : 0.0f;

    // 方角且无描边：保持原来的单次填充，不为整屏背景分配离屏纹理
    if (radiusPx <= 0.0f && borderPx <= 0.0f)
    {
        OpenCoreManagers::GFXManager.FillRect(Rect(bounds), m_backgroundColor);
        return;
    }

    // 方角描边：四条边直接铺，避免为纯矩形生成缓存
    if (radiusPx <= 0.0f)
    {
        if (hasFill)
            OpenCoreManagers::GFXManager.FillRect(Rect(bounds), m_backgroundColor);

        const float bw = std::min(borderPx,
                                  std::min(static_cast<float>(bounds.w),
                                           static_cast<float>(bounds.h)) *
                                      0.5f);
        const float x = static_cast<float>(bounds.x);
        const float y = static_cast<float>(bounds.y);
        const float w = static_cast<float>(bounds.w);
        const float h = static_cast<float>(bounds.h);

        OpenCoreManagers::GFXManager.FillRect(
            Rect{x, y, w, bw}, m_borderColor);
        OpenCoreManagers::GFXManager.FillRect(
            Rect{x, y + h - bw, w, bw}, m_borderColor);
        OpenCoreManagers::GFXManager.FillRect(
            Rect{x, y + bw, bw, h - bw * 2.0f}, m_borderColor);
        OpenCoreManagers::GFXManager.FillRect(
            Rect{x + w - bw, y + bw, bw, h - bw * 2.0f}, m_borderColor);
        return;
    }

    ensureBackgroundCache(bounds, radiusPx, borderPx);
    if (!m_backgroundCache || !m_backgroundCache->get())
        return;

    Rect dst(bounds);
    m_backgroundCache->Draw(nullptr, &dst, 0.0, nullptr,
                            static_cast<uint8_t>(VState->getAlpha()));
}

void UIElement::ensureBackgroundCache(const SDL_Rect &bounds, float radiusPx,
                                      float borderPx)
{
    const bool sizeMatches =
        m_backgroundCache && m_backgroundCache->width == bounds.w &&
        m_backgroundCache->height == bounds.h;

    if (!m_backgroundDirty && sizeMatches)
        return;

    // 缩放动画期间尺寸每帧都变，先复用旧缓存，动画结束再按最终尺寸重建
    if (!m_backgroundDirty && !sizeMatches && !isAnimeFinished())
        return;

    RoundedRect::Style style;
    style.fill        = m_backgroundColor;
    style.border      = m_borderColor;
    style.borderWidth = borderPx;
    style.radius      = radiusPx;

    m_backgroundCache = RoundedRect::Build(bounds, style);
    if (m_backgroundCache && m_backgroundCache->get())
        m_backgroundDirty = false;
}

} // namespace OpenUI