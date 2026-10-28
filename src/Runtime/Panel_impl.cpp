#include "Runtime/Graphics/UI/Panel.hpp"

namespace OpenUI
{

Panel::Panel(std::string_view id, uint8_t layer)
    : UIElement(id, layer, nullptr)
{
    // Panel 没有自身内容，跳过基类的离屏内容纹理分配
    m_textureDirty = false;
}

void Panel::onUpdate(float totalTime)
{
    // 窗口缩放会把 m_textureDirty 重新置真，这里每帧压掉，避免整屏纹理
    m_textureDirty = false;
    UIElement::onUpdate(totalTime);
}

} // namespace OpenUI
