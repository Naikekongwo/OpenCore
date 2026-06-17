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

using std::fstream;
using std::initializer_list;
using std::mutex;
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

    bool registerResource(ResourceType rType, string_view name,
                          string_view filePath);
    bool registerResource(ResourceNode resource);
    bool registerResources(initializer_list<ResourceNode> resources);

    // ──────────────────────────────────────────────
    //  资源访问接口（取代 ResourceManager）
    // ──────────────────────────────────────────────

    /**
     * @brief 获取已加载的纹理，若未加载则触发异步加载并阻塞等待。
     * @param name 资源名称（注册时使用的 name）
     * @return shared_ptr<SDL_Texture>，失败时返回 nullptr
     */
    shared_ptr<SDL_Texture> getTexture(string_view name);

    /**
     * @brief 获取已加载的字体，若未加载则触发异步加载并阻塞等待。
     * @param name   资源名称
     * @param ptsize 字体大小（磅值）
     * @return shared_ptr<TTF_Font>，失败时返回 nullptr
     */
    shared_ptr<TTF_Font> getFont(string_view name, int ptsize);

    /**
     * @brief 清空所有资源缓存，下次 getTexture/getFont 会重新加载
     */
    void clearCache();

  private:
    // ── 内部状态 ──
    string               packageName;
    bool                 packedMode = false;
    Timer               *timer      = nullptr;
    ResourceInfo         resourceInfo;
    vector<ResourceNode> resourceManifestBuffer;
    SDL_Renderer        *renderer_ = nullptr;

    // ── 资源缓存（已就绪） ──
    std::unordered_map<string, shared_ptr<SDL_Texture>> textureCache_;
    std::unordered_map<string, shared_ptr<TTF_Font>>    fontCache_;

    // ── 加载中去重（按类型分离避免 key 碰撞） ──
    std::unordered_map<string, std::shared_future<void>> pendingTextures_;
    std::unordered_map<string, std::shared_future<void>> pendingFonts_;

    std::mutex cacheMutex_;

    static constexpr float EVICT_TTL = 10.0f; // 资源删除轮询间隔

    // ── 辅助方法 ──
    void evictStaleEntries();

    bool contains(ResourceNode target, bool nameOnly = false);
    bool generatePackage(const path &manifestPath, bool cleanup = true);

    static path       getManifestPath(string_view packageName, bool packed);
    ResourceNode     *findNode(string_view name);
    std::vector<char> extractResourceData(const ResourceNode &node);

    std::shared_future<void> requestTextureLoad(string_view name);
    std::shared_future<void> requestFontLoad(string_view name, int ptsize);
};