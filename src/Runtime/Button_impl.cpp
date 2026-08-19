
#include "Runtime/Graphics/UI/Button.hpp"
#include "Core/Math/OpenCore_Color.hpp"
#include "Core/Math/OpenCore_Rect.hpp"
#include "OpenCore.hpp"

#include <SDL3/SDL_render.h>
#include <memory>

Button::Button(std::string_view id, uint8_t layer, shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
}

size_t Button::getMaxFrames() const
{
    if (texture && texture->get())
        return texture->Size();
    return 0;
}

void Button::onClick(Event *event, const SDL_Point &mousePos)
{
    if (m_onClickCallback)
        m_onClickCallback();
}

void Button::onUpdate(float totalTime)
{
    if (!isAnimeFinished())
    {
        IDrawableObject::onUpdate(totalTime);
    }

    // 将交互状态映射到纹理帧（Normal / Hovered / Pressed）
    size_t maxFrames = getMaxFrames();
    if (maxFrames == 0)
        return;

    int stateIndex = static_cast<int>(m_interactionState);
    if (static_cast<size_t>(stateIndex) >= maxFrames)
        stateIndex = 0;

    VState->frameIndex = stateIndex;
}

void Button::Draw()
{
    UIElement::Draw();

    Rect dstRect = getLogicalBounds();
    Rect VRect   = OpenCoreManagers::GFXManager.getInstance().getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    if (!texture || !texture->get())
        return;

    size_t maxFrames = getMaxFrames();
    if (maxFrames == 0)
        return;

    // 帧索引越界时回退到第 0 帧（>= 避免 off-by-one）
    auto frameIndex =
        (VState->getFrameIndex() >= maxFrames) ? 0 : VState->getFrameIndex();

    Rect srcRect = texture->getSubRect(frameIndex);
    texture->Draw(&srcRect, &dstRect, VState->getAngle(), nullptr,
                  static_cast<uint8_t>(VState->getAlpha()));
}