#include "OpenCore.hpp"

ToolMenu::ToolMenu(std::string_view id, short layer)
    : UIElement(id, layer, nullptr)
{
}

ToolMenu::~ToolMenu() {}

void ToolMenu::onUpdate(float totalTime) { UIElement::onUpdate(totalTime); }

void ToolMenu::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
}

bool ToolMenu::onDestroy() { return true; }
