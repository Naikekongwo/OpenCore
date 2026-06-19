/**
 * @file PackageManager.hpp
 * @author your name (you@domain.com)
 * @brief 资源包管理器类
 * @version 0.1
 * @date 2026-06-15
 *
 * @copyright Copyright (c) 2026
 *
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
    RscMusic,
    RscFont
};

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

    /// 序列化为一行字符串（CSV格式）
    string serialize() const
    {
        string typeStr;
        switch (rType)
        {
        case RscTexture:
            typeStr = "Texture";
            break;
        case RscMusic:
            typeStr = "Music";
            break;
        case RscFont:
            typeStr = "Font";
            break;
        }
        return typeStr + "," + name + "," + filePath + "," +
               std::to_string(startIndex) + "," + std::to_string(endIndex);
    }

    /// 从一行字符串反序列化
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

        if (typeStr == "Texture")
            node.rType = RscTexture;
        else if (typeStr == "Music")
            node.rType = RscMusic;
        else if (typeStr == "Font")
            node.rType = RscFont;

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

    /// 清除内存中加载的所有资源缓存
    void clearCache();

    /// 注册计时器
    void setTimer(Timer *timer) { this->timer = timer; }

    /// 注册相关信息的方法
    bool registerTextureMeta(TextureMeta meta);
    bool registerResource(ResourceType rType, string_view name,
                          string_view filePath);
    bool registerResource(ResourceNode resource);
    bool registerResources(initializer_list<ResourceNode> resources);

    /// Package中的元资源获取方法
    shared_ptr<SDL_Texture> getTexture(string_view name);
    shared_ptr<SDL_Texture> getTextureAsync(string_view name);
    shared_ptr<TTF_Font>    getFont(string_view name, int ptsize);

    /// 注册信息的资源获取方法
    optional<TextureMeta> queryTextureMeta(string_view name) const;
    shared_ptr<Texture>   getTextureObject(string_view name);
    shared_ptr<Texture>   getTextureObject(TextureMeta meta);

  private:
    string       packageName;
    bool         packedMode = false;
    Timer       *timer      = nullptr;
    ResourceInfo resourceInfo;

    /// 注册信息缓存
    /// 资源节点的注册缓存
    /// 纹理包装类元数据的缓存
    vector<ResourceNode>                    resourceManifestBuffer;
    std::unordered_map<string, TextureMeta> metaRegistry_;

    /// 资源元数据缓存区域
    /// 纹理区域
    /// 字体区域
    std::unordered_map<string, shared_ptr<SDL_Texture>> textureCache_;
    std::unordered_map<string, shared_ptr<TTF_Font>>    fontCache_;

    /// 纹理包装的数据缓存区域
    std::unordered_map<string, shared_ptr<Texture>> textureObjCache_;

    /// 资源加载过程缓存区域
    /// 用以保证幂等性，防止多次加载资源
    std::unordered_map<string, std::shared_future<void>> pendingTextures_;
    std::unordered_map<string, std::shared_future<void>> pendingFonts_;

    /// 缓存区域的锁
    std::mutex cacheMutex_;

    /// 常量定义
    static constexpr float EVICT_TTL   = 10.0f; // 清单条目过期时间
    static constexpr float GC_INTERVAL = 20.0f; // 整体淘汰回收间隔（2倍 TTL）

    /// 上一次调用GC系统的时间
    float lastGcTime_ = 0.0f;

    /// 垃圾回收方法
    void evictStaleEntries();

    bool contains(ResourceNode target, bool nameOnly = false);
    bool generatePackage(const path &manifestPath, bool cleanup = true);

    /// 辅助方法：用于获取当前应用的清单文件路径
    static path getManifestPath(string_view packageName, bool packed);

    /// 查询对应的ResourceNode的方法
    ResourceNode *findNode(string_view name);

    /// 从Package中读取二进制数据
    std::vector<char> extractResourceData(const ResourceNode &node);

    /// 异步加载资源的方法
    std::shared_future<void> requestTextureLoad(string_view name);
    std::shared_future<void> requestFontLoad(string_view name, int ptsize);
};