/**
 * @file PackageManager.hpp
 * @brief 资源包管理器（OpenCore 26.1 引入）
 */
#pragma once

#include <filesystem>
#include <fstream>
#include <future>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Asset/TextureLoader.hpp"
#include "Core/Info/ResourceInfo.hpp"
#include "Core/Thread/ThreadManager.hpp"
#include "Core/Timer.hpp"
#include "Runtime/Graphics/IDrawableObject/Texture.hpp"

using std::fstream;
using std::initializer_list;
using std::mutex;
using std::optional;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;
using std::filesystem::path;

enum ResourceType
{
    RscTexture,
    RscAudio, // 原 RscMusic 更名，统一音频资源
    RscFont,
    RscAsset // 通用二进制资源（JSON/配置/着色器/脚本等）
};

/** @brief 资源类型与清单字符串的映射（CSV 序列化，新增类型只需在此各加一项） */
inline string_view resourceTypeToString(ResourceType type)
{
    switch (type)
    {
    case RscTexture:
        return "Texture";
    case RscAudio:
        return "Audio";
    case RscFont:
        return "Font";
    case RscAsset:
        return "Asset";
    }
    return "";
}

/** @brief 清单字符串 → 资源类型，未知类型返回 nullopt */
[[nodiscard]] inline optional<ResourceType>
resourceTypeFromString(string_view str)
{
    if (str == "Texture")
        return RscTexture;
    if (str == "Audio")
        return RscAudio;
    if (str == "Font")
        return RscFont;
    if (str == "Asset")
        return RscAsset;
    return std::nullopt;
}

/**
 * @brief 资源节点，用于保存索引文件
 *
 */
struct ResourceNode
{
    ResourceType rType;
    string       name;
    string       filePath;       // 此参数在生成资源包时舍弃
    int          startIndex = 0; // 此参数不由外部显式注册
    int          endIndex   = 0; // 此参数不由外部显式注册
    float expireTime = 0.0f;     // 此参数与序列化、反序列化无关（不储存到本地）

    /** @brief 序列化为一行字符串（CSV 格式） */
    string serialize() const
    {
        return string(resourceTypeToString(rType)) + "," + name + "," +
               filePath + "," + std::to_string(startIndex) + "," +
               std::to_string(endIndex);
    }

    /** @brief 从一行字符串反序列化 */
    static ResourceNode deserialize(string_view line)
    {
        ResourceNode node;
        // 按逗号分割
        auto firstComma  = line.find(',');
        auto secondComma = line.find(',', firstComma + 1);
        auto thirdComma  = line.find(',', secondComma + 1);
        auto fourthComma = line.find(',', thirdComma + 1);

        string typeStr(line.substr(0, firstComma));
        node.name = line.substr(firstComma + 1, secondComma - firstComma - 1);
        node.filePath =
            line.substr(secondComma + 1, thirdComma - secondComma - 1);
        node.startIndex = std::stoi(
            string(line.substr(thirdComma + 1, fourthComma - thirdComma - 1)));
        node.endIndex = std::stoi(string(line.substr(fourthComma + 1)));

        if (auto type = resourceTypeFromString(typeStr))
            node.rType = *type;

        return node;
    }

    bool operator==(const ResourceNode &other) const
    {
        return rType == other.rType && name == other.name &&
               filePath == other.filePath && startIndex == other.startIndex &&
               endIndex == other.endIndex;
    }
};

/**
 * @brief 纹理元信息，描述纹理的网格分割（列数 × 行数）
 */
struct TextureMeta
{
    string  textureName;
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

/**
 * @brief 资源包管理器类
 * @details OpenCore 26.1版本引入的新资源管理器类
 */
class PackageManager final
{
  public:
    explicit PackageManager(string_view  pName,
                            ResourceInfo resInfo = ResourceInfo{});
    ~PackageManager() = default;

    /**
     * @brief 设置资源包的前缀名称
     *
     * @param pName
     */
    void setPackageName(string_view pName) { this->packageName = pName; }

    bool onEnter();

    /**
     * @brief 资源管理器的更新方法，用于判断资源过期等内容
     *
     */
    void onUpdate();
    void onDestroy();

    /** @brief 清除内存中加载的所有资源缓存 */
    void clearCache();

    /** @brief 注册计时器 */
    void setTimer(Timer *timer) { this->timer = timer; }

    /** @brief 注册资源/元信息的方法 */
    bool registerTextureMeta(TextureMeta meta);
    bool registerResource(ResourceType rType, string_view name,
                          string_view filePath);
    bool registerResource(ResourceNode resource);
    bool registerResources(initializer_list<ResourceNode> resources);

    /**
     * @brief 注册整个文件夹（仅扫描直接子文件，不递归子目录），按扩展名自动推断资源类型
     * @details 遍历 folderPath 下的直接文件，以「文件名（去扩展名）」作为资源名注册，
     *          并根据扩展名自动归类：
     *          - 图片（.png/.jpg/.jpeg/.bmp/.webp/.gif）→ RscTexture
     *          - 音频（.mp3/.ogg/.wav/.flac）           → RscAudio
     *          - 字体（.ttf/.otf）                      → RscFont
     *          未知扩展名的文件会被跳过；不同子目录下同名文件以先注册者为准
     *          （与 registerResource 的 contains 去重行为一致）。
     * @param folderPath 待注册的文件夹路径
     * @return 所有文件注册成功（含跳过未知类型）返回 true
     */
    bool registerFolder(string_view folderPath);

    /** @brief Package 中的元资源获取方法 */
    shared_ptr<SDL_Texture> getTexture(string_view name);
    shared_ptr<SDL_Texture> getTextureAsync(string_view name);
    shared_ptr<TTF_Font>    getFont(string_view name, int ptsize);

    /** @brief 通用二进制资源获取（RscAsset） */
    shared_ptr<vector<char>> getAsset(string_view name);
    shared_ptr<vector<char>> getAssetAsync(string_view name);

    /** @brief 音频资源获取（RscAudio，返回原始字节供解码） */
    shared_ptr<vector<char>> getAudio(string_view name);
    shared_ptr<vector<char>> getAudioAsync(string_view name);

    /** @brief 注册信息的资源获取方法 */
    optional<TextureMeta> queryTextureMeta(string_view name) const;
    shared_ptr<Texture>   getTextureObject(string_view name);
    shared_ptr<Texture>   getTextureObject(TextureMeta meta);

  private:
    string       packageName;
    bool         packedMode = false;
    Timer       *timer      = nullptr;
    ResourceInfo resourceInfo;

    // 注册信息缓存：资源节点 + 纹理元数据
    vector<ResourceNode>                    resourceManifestBuffer;
    std::unordered_map<string, TextureMeta> metaRegistry_;

    // 资源缓存：纹理 / 字体 / 通用二进制（Asset/Audio）
    std::unordered_map<string, shared_ptr<SDL_Texture>>  textureCache_;
    std::unordered_map<string, shared_ptr<TTF_Font>>     fontCache_;
    std::unordered_map<string, shared_ptr<vector<char>>> dataCache_;

    // 纹理包装数据缓存
    std::unordered_map<string, shared_ptr<Texture>> textureObjCache_;

    // 资源加载过程缓存（保证幂等，防止重复加载）
    std::unordered_map<string, std::shared_future<void>> pendingTextures_;
    std::unordered_map<string, std::shared_future<void>> pendingFonts_;
    std::unordered_map<string, std::shared_future<void>> pendingData_;

    // 缓存锁
    std::mutex cacheMutex_;

    // 常量
    static constexpr float EVICT_TTL   = 10.0f; // 清单条目过期时间
    static constexpr float GC_INTERVAL = 20.0f; // 整体淘汰回收间隔（2倍 TTL）

    // 上次 GC 时间
    float lastGcTime_ = 0.0f;

    /** @brief 淘汰过期缓存条目 */
    void evictStaleEntries();

    bool contains(ResourceNode target, bool nameOnly = false);
    bool generatePackage(const path &manifestPath, bool cleanup = true);

    /** @brief 获取当前应用的清单文件路径 */
    static path getManifestPath(string_view packageName, bool packed);

    /** @brief 查询对应的 ResourceNode */
    ResourceNode *findNode(string_view name);

    /** @brief 从 Package 中读取二进制数据 */
    std::vector<char> extractResourceData(const ResourceNode &node);

    /** @brief 异步加载资源 */
    std::shared_future<void> requestTextureLoad(string_view name);
    std::shared_future<void> requestFontLoad(string_view name, int ptsize);
    std::shared_future<void> requestDataLoad(string_view  name,
                                             ResourceType rType);

    /** @brief 通用二进制资源获取核心（缓存 → 异步加载） */
    shared_ptr<vector<char>> getData(string_view name, ResourceType rType,
                                     bool blocking);
};