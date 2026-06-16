#pragma once

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

class ToolMenu : public UIElement
{
  public:
    ToolMenu(const string &id, short layer, shared_ptr<Texture> texture);
    ~ToolMenu() override;

    void onUpdate(float totalTime) override;
    void parseEvents(Event *event, float totalTime) override;
    void Draw() override;
    bool onDestroy() override;

  private:
};