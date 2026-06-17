// UIFactory.inl
// UI的工厂方法
#pragma once
#include "OpenCore.hpp"
#include <memory>

/// @brief 通过 TextureMetaManager 按名称获取纹理的辅助函数
inline shared_ptr<Texture> GetTextureByName(std::string_view name)
{
    if (name.empty())
        return nullptr;
    auto texOpt =
        OpenEngine::getInstance().getTextureMetaManager()->getTexture(name);
    return texOpt.has_value() ? texOpt.value() : nullptr;
}

// 通用版本
template <typename T>
inline unique_ptr<T> UI(const std::string &id, uint8_t layer,
                        std::string_view textureName, short frameX,
                        short frameY)
{
    return std::make_unique<T>(id, layer, GetTextureByName(textureName));
}

template <>
inline unique_ptr<FrameCounter>
UI<FrameCounter>(const std::string &id, uint8_t layer,
                 std::string_view textureName, short reserve0, short reserve1)
{
    return std::make_unique<FrameCounter>(id, layer);
}

template <>
inline unique_ptr<BaseBackground>
UI<BaseBackground>(const std::string &id, uint8_t layer,
                   std::string_view textureName, short reserve0, short reserve1)
{
    return std::make_unique<BaseBackground>(id, layer,
                                            GetTextureByName(textureName));
}

template <>
inline unique_ptr<CheckBox> UI<CheckBox>(const std::string &id, uint8_t layer,
                                         std::string_view textureName,
                                         short reserve1, short reserve0)
{
    return std::make_unique<CheckBox>(id, layer, GetTextureByName(textureName));
}

template <>
inline unique_ptr<TextArea> UI<TextArea>(const std::string &id, uint8_t layer,
                                         std::string_view fontName,
                                         short reserve1, short reserve0)
{
    return std::make_unique<TextArea>(id, layer, fontName);
}

template <>
inline unique_ptr<TypeWriter>
UI<TypeWriter>(const std::string &id, uint8_t layer, std::string_view fontName,
               short reserve1, short reserve0)
{
    return std::make_unique<TypeWriter>(id, layer, fontName);
}
