#pragma once

#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

#include <memory>
#include <string>

using std::string;
using std::unique_ptr;

class TextArea : public UIElement
{
  public:
    TextArea(const string &id, uint8_t layer, std::string_view fontName);

    void parseEvents(Event *event, float totalTime) override;
    void onUpdate(float totalTime) override;
    void Draw() override;

    void setText(string_view textContent);
    void setFontSize(short fontSize);
    void setShadow(bool enableTag, int shadowOffset);
    void alignCenter(bool tag) { m_aligncenter = tag; }

    void setTextColor(uint8_t r, uint8_t g, uint8_t b)
    {
        m_textAttr.color = Color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        m_textureDirty   = true;
    }

  private:
    bool generateTexture(SDL_Texture *texture) override;

    TextAttribute m_textAttr;
    string        m_textContent;
    bool          m_aligncenter = false;

    float transparency = 0.69f; ///< 阴影透明度系数
};