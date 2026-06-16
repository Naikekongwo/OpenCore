// UIFactory.inl
// UI的工厂方法
#pragma once
#include "OpenCore.hpp"
#include <memory>

// 通用版本
template <typename T>
inline unique_ptr<T> UI(const std::string &id, uint8_t layer, short texID,
                        short frameX, short frameY)
{
    return std::make_unique<T>(
        id, layer,
        std::make_shared<Texture>(
            frameX, frameY, OpenCoreManagers::ResManager.GetTexture(texID)));
}

template <>
inline unique_ptr<FrameCounter> UI<FrameCounter>(const std::string &id,
                                                 uint8_t layer, short texID,
                                                 short reserve0, short reserve1)
{
    return std::make_unique<FrameCounter>(id, layer, nullptr);
}

template <>
inline unique_ptr<BaseBackground>
UI<BaseBackground>(const std::string &id, uint8_t layer, short texID,
                   short reserve0, short reserve1)
{
    return std::make_unique<BaseBackground>(
        id, layer,
        std::make_shared<Texture>(
            3, 3, OpenCoreManagers::ResManager.GetTexture(texID)));
}

template <>
inline unique_ptr<Scrollbar> UI<Scrollbar>(const std::string &id, uint8_t layer,
                                           short backTexID, short buttTexID,
                                           short reserve0)
{
    return std::make_unique<Scrollbar>(id, layer, backTexID, buttTexID);
}

template <>
inline unique_ptr<CheckBox> UI<CheckBox>(const std::string &id, uint8_t layer,
                                         short TexID, short reserve1,
                                         short reserve0)
{
    return std::make_unique<CheckBox>(
        id, layer,
        std::make_shared<Texture>(
            1, 2, OpenCoreManagers::ResManager.GetTexture(TexID)));
}

template <>
inline unique_ptr<TextArea> UI<TextArea>(const std::string &id, uint8_t layer,
                                         short fontID, short reserve1,
                                         short reserve0)
{
    return std::make_unique<TextArea>(id, layer, fontID);
}

template <>
inline unique_ptr<TypeWriter> UI<TypeWriter>(const std::string &id,
                                             uint8_t layer, short fontID,
                                             short reserve1, short reserve0)
{
    return std::make_unique<TypeWriter>(id, layer, fontID);
}
