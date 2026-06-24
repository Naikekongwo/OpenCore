
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