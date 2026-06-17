#include "Runtime/Graphics/Manager/TextureMeta.hpp"
#include "OpenCore.hpp"
#include <memory>
#include <optional>

TextureMetaManager::TextureMetaManager()
{
    _metaRegistry.clear();
    _textureCache.clear();
}

bool TextureMetaManager::registerTextureMeta(TextureMeta meta)
{
    if (meta.textureName.empty())
        return false;
    _metaRegistry[meta.textureName] = meta;
    return true;
}

std::optional<shared_ptr<Texture>>
TextureMetaManager::getTexture(string_view name)
{
    string key(name);

    // 查缓存
    auto cacheIt = _textureCache.find(key);
    if (cacheIt != _textureCache.end())
        return cacheIt->second;

    // 查元信息
    auto metaIt = _metaRegistry.find(key);
    if (metaIt == _metaRegistry.end())
        return std::nullopt;

    const TextureMeta &meta = metaIt->second;

    // 从 PackageManager 加载纹理
    auto *pkg = OpenEngine::getInstance().getPackageManager();
    if (!pkg)
        return std::nullopt;

    auto sdlTex = pkg->getTexture(name);
    if (!sdlTex)
        return std::nullopt;

    auto texture =
        std::make_shared<Texture>(meta.cols, meta.rows, std::move(sdlTex));
    if (!texture)
        return std::nullopt;

    _textureCache[key] = texture;
    return texture;
}