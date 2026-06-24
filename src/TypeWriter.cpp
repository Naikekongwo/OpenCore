#include "Runtime/Graphics/UI/TypeWriter.hpp"

#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include "Runtime/Graphics/UI/BaseBackground.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdint>
#include <memory>
#include <string>

TypeWriter::TypeWriter(string_view id, uint8_t layer, std::string_view fontName)
    : UIElement(id.data(), layer, nullptr)
{
    this->fontName = fontName;

    LOG("初始化成功，ID {}, 字体名称 {}", id.data(), fontName);
}

void TypeWriter::Draw()
{
    if (!m_textureCache || m_textureDirty)
        return;

    if (status < TypeWriterStatus::Ready)
        return;

    Rect  dstRect = getLogicalBounds();
    auto &GFX     = OpenCoreManagers::GFXManager;
    Rect  VRect   = GFX.getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    // <真正的绘制逻辑>

    if (status != TypeWriterStatus::Creating && (m_baseBackground != nullptr))
    {
        m_baseBackground->Draw();
    }

    // 在此处处理打字机效果
    if (m_textureCache)
        m_textureCache->Draw(nullptr, &dstRect, 0.0, nullptr,
                             static_cast<uint8_t>(VState->getAlpha()));
}

void TypeWriter::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
    // m_textureDirty 已在基类 parseEvents 中由 WINDOW_RESIZED 设置

    if (status != TypeWriterStatus::Creating)
    {
        if (m_baseBackground)
            m_baseBackground->parseEvents(event, totalTime);
    }
}

void TypeWriter::setText(string_view textContent)
{
    // 设置对应的文字
    m_textContent  = textContent;
    m_textureDirty = true;
}

void TypeWriter::setFontSize(short fontSize)
{
    m_fontSize     = fontSize;
    m_textureDirty = true;
}

void TypeWriter::onUpdate(float totalTime)
{
    UIElement::onUpdate(totalTime);
    if (status == TypeWriterStatus::Creating)
    {
        if (m_enableBackground)
        {
            m_baseBackground =
                UI<BaseBackground>("baseBackground", 0, "", NULL, NULL);

            m_baseBackground->setNativeScale(10);
            m_baseBackground->Configure()
                .Parent(this)
                .Anchor(AnchorPoint::Center)
                .Scale(1.0f, 1.0f)
                .Posite(0.5f, 0.5f);
        }

        status = TypeWriterStatus::Ready;
    }
    if (m_baseBackground)
    {
        m_baseBackground->onUpdate(totalTime);
    }
}

void TypeWriter::setShadow(bool enableTag, int shadowOffset)
{
    m_shadowEnable = enableTag;
    m_shadowOffset = shadowOffset;
    m_textureDirty = true;
}

bool TypeWriter::generateTexture()
{
    m_parsedLines.clear();

    if (VState->transparency <= 0.0f)
        return false;

    auto &GFX       = OpenCoreManagers::GFXManager.getInstance();
    Rect  container = getLogicalBounds();

    auto font = OpenEngine::getInstance().getPackageManager()->getFont(
        fontName, m_fontSize);
    if (!font)
        return false;

    m_textureCache = std::make_shared<Texture>(
        static_cast<uint16_t>(container.w), static_cast<uint16_t>(container.h),
        size_t(1), size_t(1));
    if (!m_textureCache || !m_textureCache->get())
        return false;

    GFX.setRenderTarget(m_textureCache->get());

    SDL_SetRenderDrawBlendMode(GFX.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GFX.getRenderer(), 0, 0, 0, 0);
    SDL_RenderClear(GFX.getRenderer());

    auto utf8Len = [](unsigned char c) -> size_t
    {
        if ((c & 0x80) == 0x00)
            return 1;
        if ((c & 0xE0) == 0xC0)
            return 2;
        if ((c & 0xF0) == 0xE0)
            return 3;
        return 4;
    };

    auto measure = [&](const std::string &s) -> float
    {
        SDL_Surface *surf = TTF_RenderText_Blended(
            font.get(), s.c_str(), s.length(),
            {255, 255, 255, static_cast<uint8_t>(VState->getAlpha())});
        if (!surf)
            return 0;

        float scale = m_fontSize * 1.0f / surf->h;
        float w     = surf->w * scale;

        SDL_DestroySurface(surf);
        return w;
    };

    std::string line;

    auto pushLine = [&]()
    {
        m_parsedLines.push_back(line);
        line.clear();
    };

    for (size_t i = 0; i < m_textContent.size();)
    {
        size_t      len = utf8Len((unsigned char)m_textContent[i]);
        std::string ch  = m_textContent.substr(i, len);

        if (ch == "\n")
        {
            pushLine();
            i += len;
            continue;
        }

        std::string test = line + ch;

        if (!line.empty() && measure(test) > container.w)
        {
            pushLine();
            continue;
        }

        line = test;
        i += len;
    }

    if (!line.empty())
        pushLine();

    int y = 0;

    for (auto &l : m_parsedLines)
    {
        SDL_Surface *surf = TTF_RenderText_Blended(
            font.get(), l.c_str(), l.length(), {255, 255, 255, 255});

        if (!surf)
        {
            GFX.setRenderTarget(nullptr);
            return false;
        }

        SDL_Texture *tex =
            SDL_CreateTextureFromSurface(GFX.getRenderer(), surf);

        if (!tex)
        {
            SDL_DestroySurface(surf);
            GFX.setRenderTarget(nullptr);
            return false;
        }

        float scale = m_fontSize * 1.0f / surf->h;

        Rect dst;
        dst.w = surf->w * scale;
        dst.h = m_fontSize;
        dst.x = m_aligncenter ? (container.w - dst.w) * 0.5f : 0;
        dst.y = y;

        if (y + dst.h > container.h)
        {
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
            break;
        }

        if (m_shadowEnable)
        {
            Rect s = dst;
            s.x += m_shadowOffset;
            s.y += m_shadowOffset;

            SDL_SetTextureColorMod(tex, 0, 0, 0);
            auto shadowTex = std::make_shared<Texture>(
                1, 1, std::shared_ptr<SDL_Texture>(tex, [](SDL_Texture *) {}));
            shadowTex->Draw(nullptr, &s, 0.0f, nullptr, 180);
            SDL_SetTextureColorMod(tex, 255, 255, 255);
        }

        {
            auto lineTex = std::make_shared<Texture>(
                1, 1, std::shared_ptr<SDL_Texture>(tex, [](SDL_Texture *) {}));
            lineTex->Draw(nullptr, &dst, 0.0f, nullptr);
        }

        SDL_DestroyTexture(tex);
        SDL_DestroySurface(surf);

        y += m_fontSize + lineGap;
    }

    GFX.setRenderTarget(nullptr);

    status = TypeWriterStatus::Ready;

    return true;
}