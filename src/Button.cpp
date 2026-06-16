
#include "OpenCore.hpp"
#include "Runtime/Animation/AnimationPipeline.hpp"

#include "Core/Math/OpenCore_Rect.hpp"

Button::Button(const std::string &id, uint8_t layer,
               shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
}

void Button::parseEvents(Event *event, float totalTime)
{
    const SDL_Event &sdlEvent = event->GetSDLEvent();
    Point            mousePos{};
    Rect             bounds = getPhysicalBounds();

    switch (sdlEvent.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
    {
        mousePos.x = static_cast<int>(sdlEvent.motion.x);
        mousePos.y = static_cast<int>(sdlEvent.motion.y);

        if (!PointInRect(mousePos, bounds))
            State = ButtonState::Normal;
        else
            State = (State == ButtonState::Pressed) ? ButtonState::Pressed
                                                    : ButtonState::Hovered;
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        if (sdlEvent.button.button == SDL_BUTTON_LEFT)
        {
            mousePos.x = static_cast<int>(sdlEvent.button.x);
            mousePos.y = static_cast<int>(sdlEvent.button.y);

            if (PointInRect(mousePos, bounds))
            {
                State = ButtonState::Pressed;
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        if (sdlEvent.button.button == SDL_BUTTON_LEFT)
        {
            mousePos.x = static_cast<int>(sdlEvent.button.x);
            mousePos.y = static_cast<int>(sdlEvent.button.y);

            if (PointInRect(mousePos, bounds) && State == ButtonState::Pressed)
            {
                if (onClick)
                    onClick();

                State = ButtonState::Hovered;
            }
            else
            {

                State = ButtonState::Normal;
            }
        }
        break;
    }

    default:
        break;
    }
}

void Button::onUpdate(float totalTime)
{
    UIElement::onUpdate(totalTime);

    if (!texture)
        return;
    int maxFrame = texture->Size();

    int stateIndex = static_cast<int>(State);

    if (stateIndex >= maxFrame)
        stateIndex = 0;

    VState->frameIndex = stateIndex;
}

void Button::Draw()
{
    Rect dstRect = getLogicalBounds();
    if (!texture->get())
        return;

    auto &GFX   = OpenCoreManagers::GFXManager.getInstance();
    Rect  VRect = GFX.getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    SDL_SetTextureAlphaMod(texture->get(), VState->getAlpha());

    auto frameIndex = (VState->getFrameIndex() > texture->Size())
                          ? 0
                          : VState->getFrameIndex();

    Rect srcRect = texture->getSubRect(frameIndex);

    GFX.Draw(texture->get(), &srcRect, &dstRect, VState->getAngle(), NULL);
}