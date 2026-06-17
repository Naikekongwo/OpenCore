#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "Runtime/Graphics/IDrawableObject/Texture.hpp"

using std::optional;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::unordered_map;

struct TextureMeta
{
    string  textureName; // PackageManager 中注册的资源名
    uint8_t cols;
    uint8_t rows;

    TextureMeta(string_view name, uint8_t cols, uint8_t rows)
        : textureName(name), cols(cols), rows(rows)
    {
    }

    TextureMeta(string_view name) : textureName(name), cols(1), rows(1) {}

    TextureMeta() : cols(1), rows(1) {}

    bool operator==(const TextureMeta &other) const
    {
        return textureName == other.textureName && cols == other.cols &&
               rows == other.rows;
    }
};

struct Texture;

class TextureMetaManager final
{
  public:
    TextureMetaManager();

    bool registerTextureMeta(TextureMeta meta);

    optional<TextureMeta> queryTextureMeta(string_view name) const
    {
        auto it = _metaRegistry.find(string(name));
        if (it != _metaRegistry.end())
            return it->second;
        return std::nullopt;
    }

    // 从 PackageManager 获取纹理（含缓存）
    optional<shared_ptr<Texture>> getTexture(string_view name);

    /// @deprecated 兼容旧 short ID 的重载，将 ID 转字符串后查找
    optional<shared_ptr<Texture>> getTexture(short id)
    {
        return getTexture(std::to_string(id));
    }

  private:
    unordered_map<string, TextureMeta>         _metaRegistry;
    unordered_map<string, shared_ptr<Texture>> _textureCache;
};