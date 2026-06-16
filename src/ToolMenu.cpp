#include "OpenCore.hpp"

ToolMenu::ToolMenu(const string &id, short layer, shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
}

ToolMenu::~ToolMenu() {}

void ToolMenu::onUpdate(float totalTime) { UIElement::onUpdate(totalTime); }

void ToolMenu::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
}

void ToolMenu::Draw() { UIElement::Draw(); }

bool ToolMenu::onDestroy() { return true; }
