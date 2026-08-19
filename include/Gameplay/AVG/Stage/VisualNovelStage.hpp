#pragma once
/**
 * @file VisualNovelStage.hpp
 * @author Naikekongwo
 * @brief AVG 基础场景类声明
 * @version 0.1
 * @date 2026-08-18
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "World/Stage/OverlayStage.hpp"

// 关于标准的控件ID的宏定义
#define MSG_PANEL "messagePanel"
#define BTN_LOAD "btn_load"
#define BTN_SAVE "btn_save"
#define BTN_HISTORY "btn_history"
#define NAMETAG "nameTag"

/**
 * @class VisualNovelStage
 * @brief AVG（视觉小说）游戏的基础场景。
 *
 * 作为覆盖层场景承载 AVG 演出（背景、立绘、对话框、剧本推进）。
 * 派生类通过重写 initializeComponents() 搭建具体场景内容，
 * 并可在 parseEvents() 中拦截输入以推进对话或处理选择肢。
 */
class VisualNovelStage : public OverlayStage
{
  public:
    void onEnter() override;
    void onUpdate() override;
    void onRender() override;

    bool parseEvents(Event *event) override;

    void initializeComponents() override;
};