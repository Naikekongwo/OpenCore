
#include "OpenCore.hpp"
#include <memory>

Scrollbar::Scrollbar(const string &id, short layer,
                     std::string_view backTexName, std::string_view buttTexName)
    : UIElement(id, layer, nullptr)
{

    if (!backTexName.empty() && !buttTexName.empty())
    {
        this->backgroundTextureName = backTexName;
        this->buttonTextureName     = buttTexName;
    }

    value = make_shared<float>(0);
}

void Scrollbar::onEnter()
{
    if (status == ScrollStatus::Creating)
    {

        baseBack = UI<BaseBackground>("background", 1, backgroundTextureName,
                                      NULL, NULL);
        slideBar = UI<ImageBoard>("slideimg", 1, buttonTextureName, 1, 1);

        slideBar->Configure()
            .Anchor(AnchorPoint::Center)
            .Parent(this)
            .Posite(0.5f, 0.5f)
            .Scale(0.0f, 1.2f)
            .Sequence(true);

        baseBack->Configure()
            .Parent(this)
            .Posite(0.5f, 0.5f)
            .Scale(1.0f, 1.0f)
            .Sequence(true)
            .Anchor(AnchorPoint::Center);

        baseBack->setNativeScale(10);
        baseBack->onEnter();
        slideBar->onEnter();

        UpdateBar();

        status = ScrollStatus::Ready;
    }
}

void Scrollbar::Draw()
{
    UIElement::Draw();

    if (slideBar && baseBack)
    {
        ///< 都存在才会绘制
        baseBack->Draw();
        slideBar->Draw();
    }
}

void Scrollbar::onUpdate(float totalTime)
{
    if (slideBar)
    {
        slideBar->onUpdate(totalTime);
    }
}

void Scrollbar::UpdateBar() { slideBar->setPosition(*value, 0.5f); }

void Scrollbar::parseEvents(Event *event, float totalTime)
{
    if (baseBack)
        baseBack->parseEvents(event, totalTime);

    const SDL_Event &sdlEvent = event->GetSDLEvent();
    SDL_Point        mousePos{};
    SDL_Rect         bounds = getPhysicalBounds();

    switch (sdlEvent.type)
    {

    case SDL_EVENT_MOUSE_MOTION:
    {
        mousePos = {static_cast<int>(sdlEvent.motion.x),
                    static_cast<int>(sdlEvent.motion.y)};
        if (!SDL_PointInRect(&mousePos, &bounds))
        {
            status = ScrollStatus::Ready;
        }

        if (status == ScrollStatus::Following)

        {
            *value = (mousePos.x - bounds.x) / static_cast<float>(bounds.w);
            UpdateBar();
        }
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        if (sdlEvent.button.button == SDL_BUTTON_LEFT)
        {
            mousePos = {static_cast<int>(sdlEvent.button.x),
                        static_cast<int>(sdlEvent.button.y)};

            if (SDL_PointInRect(&mousePos, &bounds))
            {

                status = ScrollStatus::Following;
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        if (sdlEvent.button.button == SDL_BUTTON_LEFT)
        {
            if (status == ScrollStatus::Following)
            {
                status = ScrollStatus::Ready;
            }
        }
        break;
    }

    default:
        break;
    }
}

bool Scrollbar::onDestroy() { return true; }

void Scrollbar::onExit()
{
    slideBar.reset();
    baseBack.reset();
    value.reset();
}