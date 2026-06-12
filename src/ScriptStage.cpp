#include "OpenCore.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "World/Stage/Stage.hpp"

bool ScriptStage::parseEvents(Event *event)
{

    // 处理事件
    return true;
}

ScriptStage::ScriptStage(Timer *timer, StageManager *sController)
{
    this->timer = timer;
    this->sController = sController;

    this->stageType = overlayStage;

    this->Elements = std::make_unique<ElementManager>();
}

void ScriptStage::onEnter() { initializeComponents(); }

void ScriptStage::initializeComponents()
{
    // 初始化脚本系统
}

void ScriptStage::onUpdate() {}

void ScriptStage::onRender() {}

void ScriptStage::onExit() {}

void ScriptStage::onDestroy() {}