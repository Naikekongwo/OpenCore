// UIFactory.inl
// UI的工厂方法
#pragma once
#include "OpenCore.hpp"
#include <memory>

/// @brief 通过 PackageManager 按名称获取纹理的辅助函数
inline shared_ptr<Texture> GetTextureByName(std::string_view name)
{
    if (name.empty())
        return nullptr;
    return OpenEngine::getInstance().getPackageManager()->getTextureObject(
        name);
}

// 通用版本
template <typename T>
inline unique_ptr<T> UI(std::string_view id, uint8_t layer,
                        std::string_view textureName, uint8_t frameX,
                        uint8_t frameY)
{
    return std::make_unique<T>(
        id, layer,
        OpenEngine::getInstance().getPackageManager()->getTextureObject(
            {textureName, frameX, frameY}));
}

template <>
inline unique_ptr<OpenUI::FrameCounter>
UI<OpenUI::FrameCounter>(std::string_view id, uint8_t layer,
                         std::string_view textureName, uint8_t reserve0,
                         uint8_t reserve1)
{
    return std::make_unique<OpenUI::FrameCounter>(id, layer);
}

template <>
inline unique_ptr<OpenUI::BaseBackground>
UI<OpenUI::BaseBackground>(std::string_view id, uint8_t layer,
                           std::string_view textureName, uint8_t reserve0,
                           uint8_t reserve1)
{
    return std::make_unique<OpenUI::BaseBackground>(id, layer,
                                                    GetTextureByName(textureName));
}

template <>
inline unique_ptr<OpenUI::CheckBox>
UI<OpenUI::CheckBox>(std::string_view id, uint8_t layer,
                     std::string_view textureName, uint8_t reserve1,
                     uint8_t reserve0)
{
    return std::make_unique<OpenUI::CheckBox>(id, layer,
                                              GetTextureByName(textureName));
}

template <>
inline unique_ptr<OpenUI::Panel>
UI<OpenUI::Panel>(std::string_view id, uint8_t layer,
                  std::string_view reserveTexture, uint8_t reserve1,
                  uint8_t reserve0)
{
    return std::make_unique<OpenUI::Panel>(id, layer);
}

template <>
inline unique_ptr<OpenUI::TextArea>
UI<OpenUI::TextArea>(std::string_view id, uint8_t layer,
                     std::string_view fontName, uint8_t reserve1,
                     uint8_t reserve0)
{
    return std::make_unique<OpenUI::TextArea>(id, layer, fontName);
}

template <>
inline unique_ptr<OpenUI::TextButton>
UI<OpenUI::TextButton>(std::string_view id, uint8_t layer,
                       std::string_view text, uint8_t reserve1,
                       uint8_t reserve0)
{
    return std::make_unique<OpenUI::TextButton>(id, layer, text);
}

template <>
inline unique_ptr<OpenUI::TypeWriter>
UI<OpenUI::TypeWriter>(std::string_view id, uint8_t layer,
                       std::string_view fontName, uint8_t reserve1,
                       uint8_t reserve0)
{
    return std::make_unique<OpenUI::TypeWriter>(id, layer, fontName);
}
