#include "Core/Math/OpenCore_Color.hpp"
#include "Gameplay/AVG/OpenCoreAVGModule.hpp"
#include "Gameplay/AVG/Stage/VisualNovelStage.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/UI/ImageBoard.hpp"
#include "Runtime/Graphics/UI/TextButton.hpp"


namespace
{
// 原型阶段控件占位底色（蓝色）
const Color kUiBlue{0.20f, 0.45f, 0.85f, 1.0f};

// 原型样式：统一三态为纯白字，避免默认 Hovered 变灰干扰（纯样式，无交互反馈）
void applyPrototypeStyle(TextButton *btn)
{
    btn->setBackgroundColor(kUiBlue);

    TextAttribute attr;
    attr.option   = static_cast<TextRenderOption>(RENDER_TEXT | RENDER_SHADOW);
    attr.color    = White;
    attr.fontName = "ChineseFont";
    attr.fontSize = 36;

    btn->setNormalAttribute(attr);
}
} // namespace

void VisualNovelStage::onEnter() { initializeComponents(); }

void VisualNovelStage::onUpdate()
{
    Elements->onUpdate(timer->getTotalTime());
    pipeline.onUpdate(timer->getTotalTime());
}

void VisualNovelStage::onRender() { Elements->onRender(); }

bool VisualNovelStage::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());
    return true;
}

// 原型：姓名框 + 对话框 + 底部按钮排（仅样式，未接入功能）
void VisualNovelStage::initializeComponents()
{
    // ── 姓名框（标题栏）：贴在对话框上边缘、靠左，显示说话人姓名 ──
    auto namePlate = UI<TextButton>(NAMETAG, 99, "海己", 0, 0);
    applyPrototypeStyle(namePlate.get());
    namePlate->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::BottomCenter)
        .PositeR(0.18f, 0.56f)
        .ScaleR(0.28f, 0.055f)
        .Follow(10);
    Elements->PushElement(std::move(namePlate));

    // ── 对话框（message panel，暂时用 ImageBoard 承载）──
    auto msgPn = UI<ImageBoard>(MSG_PANEL, 98, "messagePanel", 1, 1);

    msgPn->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopCenter)
        .PositeR(0.5f, 0.57f)
        .ScaleR(0.9f, 0.0f)
        .Alpha(0.7f)
        .Follow(10);
    Elements->PushElement(std::move(msgPn));

    // ── 底部按钮排：存档 / 加载 / 历史记录（靠右下角紧密排列）──
    auto saveBtn = UI<TextButton>(BTN_SAVE, 100, "存档", 0, 0);
    applyPrototypeStyle(saveBtn.get());
    saveBtn->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::MiddleRight)
        .PositeR(0.69f, 0.95f)
        .ScaleR(0.14f, 0.05f)
        .Follow(10);
    Elements->PushElement(std::move(saveBtn));

    auto loadBtn = UI<TextButton>(BTN_LOAD, 100, "加载", 0, 0);
    applyPrototypeStyle(loadBtn.get());
    loadBtn->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::MiddleRight)
        .PositeR(0.83f, 0.95f)
        .ScaleR(0.14f, 0.05f)
        .Follow(10);
    Elements->PushElement(std::move(loadBtn));

    auto historyBtn = UI<TextButton>(BTN_HISTORY, 100, "历史记录", 0, 0);
    applyPrototypeStyle(historyBtn.get());
    historyBtn->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::MiddleRight)
        .PositeR(0.97f, 0.95f)
        .ScaleR(0.14f, 0.05f)
        .Follow(10);
    Elements->PushElement(std::move(historyBtn));
}
