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

    // 同步加载 SDL_Texture
    auto *pkg = OpenEngine::getInstance().getPackageManager();
    if (!pkg)
        return std::nullopt;

    auto sdlTex = pkg->loadTextureSync(name);
    if (!sdlTex)
        return std::nullopt;

    auto texture =
        std::make_shared<Texture>(meta.cols, meta.rows, std::move(sdlTex));
    _textureCache[key] = texture;
    return texture;
}

shared_ptr<Texture> TextureMetaManager::registerTexture(TextureMeta meta)
{
    if (meta.textureName.empty() || meta.textureName == " ")
        return nullptr;

    auto buffer_tex = getTexture(meta.textureName);
    if (buffer_tex.has_value())
    {
        auto buffer_tex_body = buffer_tex.value();
        if (buffer_tex_body->xCount == meta.cols &&
            buffer_tex_body->yCount == meta.rows)
            return buffer_tex.value();
    }

    registerTextureMeta(meta);

    // 同步加载
    auto *pkg = OpenEngine::getInstance().getPackageManager();
    if (!pkg)
        return nullptr;

    auto sdlTex = pkg->loadTextureSync(meta.textureName);
    if (!sdlTex)
        return nullptr;

    auto tex =
        std::make_shared<Texture>(meta.cols, meta.rows, std::move(sdlTex));
    _textureCache[meta.textureName] = tex;
    return tex;
}