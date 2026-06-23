
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"

SDL_Rect UIElement::getLogicalBounds()
{
    if (!VState)
    {
        LOG("UIElement::getLogicalBounds() failed: VState is nullptr");
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

UIElement::~UIElement() { m_textureCache.reset(); }

bool UIElement::onDestroy()
{
    m_textureCache.reset();
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
            generateTexture(m_textureCache->get());
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
    }
}

UIElement::UIElement(const string &id, short layer, shared_ptr<Texture> texture)
{
    this->id    = id;
    this->layer = layer;

    if (!texture)
    {
        LOG("UIElement id {} encountered a empty texture, but relax, for some "
            "element have child element it's ok to init without texture.",
            id.c_str());
    }
    else
    {
        this->texture = texture;
    }
}

void UIElement::Draw()
{
    if (color.a != 0)
    {
        SDL_Rect bounds = getLogicalBounds();
        GraphicsManager::getInstance().FillRect(bounds, color);
    }
}