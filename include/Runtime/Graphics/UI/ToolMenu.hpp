#pragma once

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include "Runtime/Graphics/UI/ToolMenu.hpp"

#include <functional>

using std::function;
using std::string;
using std::vector;

struct ToolMenuChild
{
    string           tagName;
    function<void()> onClickEvent;
};

struct ToolMenuOption
{
    string                tagName;
    vector<ToolMenuChild> children;
};

class ToolMenu : public UIElement
{
  public:
    ToolMenu(const string &id, short layer);
    ~ToolMenu() override;

    void onUpdate(float totalTime) override;
    void parseEvents(Event *event, float totalTime) override;
    void Draw() override;
    bool onDestroy() override;

  private:
    vector<ToolMenuOption> options;
};