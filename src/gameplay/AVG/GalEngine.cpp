#include "Core/Math/OpenCore_Color.hpp"
#include "Gameplay/AVG/AVGUtils.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"

void GalEngine::onEnter() { initializeComponents(); }

void GalEngine::onUpdate()
{
    Elements->onUpdate(timer->getTotalTime());
    pipeline.onUpdate(timer->getTotalTime());
}

void GalEngine::onRender() { Elements->onRender(); }

bool GalEngine::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());
    return true;
}

// 基础类默认不创建内容，派生类在此搭建具体 AVG 场景
void GalEngine::initializeComponents()
{
    auto title = UI<TextArea>("tempTitle", 99, "OpenCoreFont", 0, 0);

    title->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .ScaleR(0.5f, 0.2f)
        .PositeR(0.0f, 0.0f);

    TextAttribute attr;

    attr.option   = static_cast<TextRenderOption>(RENDER_TEXT);
    attr.color    = White;
    attr.fontSize = 72;

    title->setText("Galgame Engine Test 20260818");
    title->setAttribute(attr);
    title->align(AnchorPoint::TopLeft);

    Elements->PushElement(std::move(title));

    // auto msgPn = UI<ImageBoard>("msgPn", 99, "messagePanel", 1, 1);

    // msgPn->Configure()
    //     .Parent(nullptr)
    //     .Anchor(AnchorPoint::BottomCenter)
    //     .PositeR(0.5f, 0.95f)
    //     .ScaleR(0.9f, 0.0f)
    //     .Follow(10);
    // Elements->PushElement(std::move(msgPn));
}
