#include "Asset/PackageManager.hpp"
#include "OpenCore.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <stdexcept>

using std::endl;
using std::error_code;
namespace fs = std::filesystem;
using std::fstream;
using std::getline;
using std::initializer_list;
using std::ios;
using std::istreambuf_iterator;
using std::ostringstream;
using std::runtime_error;
using std::sort;
using std::streamsize;

PackageManager::PackageManager(string_view pName, ResourceInfo resInfo)
    : resourceInfo(resInfo)
{
    if (pName.empty())
    {
        LOG("初始化包名称遇到空值");
        throw runtime_error("packagename is null.");
    }

    if (!fs::exists("data//"))
    {
        fs::create_directory("data//");
    }

    packageName = pName;

    // 清除旧的非打包模式清单文件
    auto packageManifestFile = getManifestPath(packageName, false);
    if (fs::exists(packageManifestFile))
    {
        fs::remove(packageManifestFile);
    }

    auto packageOCData = fs::path(reinterpret_cast<const char8_t *>(
        (string("data//") + string(packageName) + string("_00.ocdata"))
            .c_str()));
    if (fs::exists(packageOCData))
    {
        packedMode = true;
        LOG("初始化成功，已加载已启用的资源包清单 {}",
            getManifestPath(packageName, true).string());
    }
    else
    {
        if (resourceInfo.packageOnly)
        {
            LOG("错误：资源包模式要求存在 .ocdata 文件，但未找到 {}",
                packageOCData.string());
            throw runtime_error(
                "packageOnly mode enabled but .ocdata file not found.");
        }

        fstream newManifest;
        newManifest.open(packageManifestFile, ios::out);
        newManifest.close();

        LOG("初始化成功，清单文件已生成到 {}", packageManifestFile.string());
    }
}

bool PackageManager::registerResource(ResourceType rType, string_view name,
                                      string_view filePath)
{
    ResourceNode resource;

    resource.rType = rType;
    resource.name = name;
    resource.filePath = filePath;

    return registerResource(resource);
}

bool PackageManager::registerResource(ResourceNode resource)
{
    if (packageName.empty())
    {
        LOG("包名为空，无法确定清单位置");
        return false;
    }

    if (packedMode)
    {
        LOG("资源包模式下清单为只读，不允许注册新资源");
        return false;
    }

    if (contains(resource))
        return true;

    resourceManifestBuffer.push_back(resource);

    auto packageManifestFile = getManifestPath(packageName, false);
    fstream manifest(packageManifestFile, ios::app | ios::binary);

    if (!manifest.good())
    {
        LOG("发生了意外的错误，该清单文件不存在 {}",
            packageManifestFile.string());
        return false;
    }

    manifest << resource.serialize() << endl;

    manifest.close();
    return true;
}

bool PackageManager::registerResources(initializer_list<ResourceNode> resources)
{
    bool result = true;
    for (const auto &entry : resources)
    {
        result = result && registerResource(entry);
    }
    return result;
}

fs::path PackageManager::getManifestPath(string_view packageName, bool packed)
{
    string path = string("data//") + string(packageName) +
                  (packed ? "_packagemanifest.txt" : "_manifest.txt");
    // 使用 char8_t* 构造函数确保 UTF-8 路径在 Windows 上正确转换
    return fs::path(reinterpret_cast<const char8_t *>(path.c_str()));
}

void PackageManager::evictStaleEntries()
{
    if (!timer)
        return;

    float now = timer->getTotalTime();

    // 按 GC_INTERVAL 间隔执行，不每帧扫描
    if (now - lastGcTime_ < GC_INTERVAL)
        return;
    lastGcTime_ = now;

    // ① 资源清单过期条目淘汰（末尾是 expireTime 最小的，从后往前删）
    while (!resourceManifestBuffer.empty() &&
           now > resourceManifestBuffer.back().expireTime)
    {
        resourceManifestBuffer.pop_back();
    }

    // ② Texture 和 Font 的自动垃圾回收
    //    遍历删除 use_count 为 1 的纹理/字体，即只有缓存自身持有的
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        for (auto it = textureCache_.begin(); it != textureCache_.end();)
        {
            if (it->second.use_count() == 1) // 仅缓存持有
                it = textureCache_.erase(it);
            else
                ++it;
        }

        for (auto it = fontCache_.begin(); it != fontCache_.end();)
        {
            if (it->second.use_count() == 1) // 仅缓存持有
                it = fontCache_.erase(it);
            else
                ++it;
        }
    }
}

void PackageManager::onUpdate() { evictStaleEntries(); }

void PackageManager::onDestroy() { resourceManifestBuffer.clear(); }

bool PackageManager::contains(ResourceNode target, bool nameOnly)
{
    auto matches = [&](const ResourceNode &entry) -> bool
    { return nameOnly ? (entry.name == target.name) : (entry == target); };

    for (auto &entry : resourceManifestBuffer)
    {
        if (matches(entry))
        {
            if (timer)
                entry.expireTime = timer->getTotalTime() + 2 * EVICT_TTL;
            // 按 expireTime 降序排序，最近使用的排前面
            sort(resourceManifestBuffer.begin(), resourceManifestBuffer.end(),
                 [](const ResourceNode &a, const ResourceNode &b)
                 { return a.expireTime > b.expireTime; });
            return true;
        }
    }

    // 缓冲中未命中，尝试从清单文件中按需加载
    auto packageManifestFile = getManifestPath(packageName, packedMode);
    fstream manifest(packageManifestFile, ios::in | ios::binary);
    if (manifest.good())
    {
        string line;
        while (getline(manifest, line))
        {
            if (!line.empty())
            {
                ResourceNode node = ResourceNode::deserialize(line);
                if (matches(node))
                {
                    // 从文件加载到内存缓冲
                    if (timer)
                        target.expireTime = timer->getTotalTime() + 15.0f;
                    resourceManifestBuffer.push_back(target);
                    sort(resourceManifestBuffer.begin(),
                         resourceManifestBuffer.end(),
                         [](const ResourceNode &a, const ResourceNode &b)
                         { return a.expireTime > b.expireTime; });
                    return true;
                }
            }
        }
        manifest.close();
    }

    return false;
}

bool PackageManager::generatePackage(const fs::path &manifestPath, bool cleanup)
{
    // 读取原始清单
    fstream manifestFile(manifestPath, ios::in | ios::binary);
    if (!manifestFile.is_open())
    {
        LOG("无法打开清单文件 {}", manifestPath.string());
        return false;
    }

    vector<ResourceNode> originalNodes;
    string line;
    while (getline(manifestFile, line))
    {
        if (!line.empty())
            originalNodes.push_back(ResourceNode::deserialize(line));
    }
    manifestFile.close();

    if (originalNodes.empty())
    {
        LOG("清单文件为空");
        return false;
    }

    // ① 检查所有资源文件是否存在
    for (const auto &node : originalNodes)
    {
        if (!fs::exists(node.filePath))
        {
            LOG("资源文件不存在 {}", node.filePath);
            return false;
        }
    }

    // ③④ 遍历manifest，生成导出节点并拼接资源数据
    vector<ResourceNode> exportNodes;
    ostringstream resourceStream;
    size_t currentOffset = 0;

    for (const auto &node : originalNodes)
    {
        fstream resFile(fs::path(node.filePath), ios::in | ios::binary);
        if (!resFile.is_open())
        {
            LOG("无法读取资源文件 {}", node.filePath);
            return false;
        }

        string content((istreambuf_iterator<char>(resFile)),
                       istreambuf_iterator<char>());
        resFile.close();

        ResourceNode exportNode;
        exportNode.rType = node.rType;
        exportNode.name = node.name;
        exportNode.filePath =
            resourceInfo.keepStructureWhenPackaging ? node.filePath : "";
        exportNode.startIndex = static_cast<int>(currentOffset);
        exportNode.endIndex = static_cast<int>(currentOffset + content.size());
        exportNodes.push_back(exportNode);

        resourceStream << content;
        currentOffset += content.size();
    }

    string resourceData = resourceStream.str();
    string packedManifestStr;
    for (const auto &node : exportNodes)
        packedManifestStr += node.serialize() + "\n";

    // 从成员变量 packageName 获取包名
    string pkgName = string(this->packageName);

    // ⑤ 写出 _packagemanifest.txt（单独文件，不嵌入 .ocdata）
    auto packedManifestPath = getManifestPath(pkgName, true);
    fstream mfOut(packedManifestPath, ios::out | ios::binary | ios::trunc);
    if (!mfOut.is_open())
    {
        LOG("无法创建打包清单文件 {}", packedManifestPath.string());
        return false;
    }
    mfOut.write(packedManifestStr.data(), packedManifestStr.size());
    mfOut.close();

    // ⑥ 组装 .ocdata 文件（仅 OCDT 魔数 + 资源数据）
    auto outputPath = fs::path(reinterpret_cast<const char8_t *>(
        (string("data//") + pkgName + string("_00.ocdata")).c_str()));
    fstream output(outputPath, ios::out | ios::binary | ios::trunc);
    if (!output.is_open())
    {
        LOG("无法创建资源包文件 {}", outputPath.string());
        return false;
    }

    output.write("OCDT", 4);                                // 魔数
    output.write(resourceData.data(), resourceData.size()); // 资源数据
    output.close();

    LOG("资源包已生成到 {}", outputPath.string());

    if (cleanup)
    {
        // 清理原始资源文件和清单
        error_code ec;
        for (const auto &node : originalNodes)
        {
            fs::remove(fs::path(node.filePath), ec);
            if (ec)
                LOG("警告：无法删除源文件 {} - {}", node.filePath,
                    ec.message());
        }
        fs::remove(manifestPath, ec);
        if (ec)
            LOG("警告：无法删除原始清单 {} - {}", manifestPath.string(),
                ec.message());
    }

    return true;
}

bool PackageManager::onEnter()
{
    // 首先检查PackageName
    if (packageName.empty() || packageName == " ")
    {
        LOG("当前游戏的游戏名称并不存在");
        return false;
    }

    // 检查当前模式
    // 我们必须假设，已经在构造器中配置好了资源包
    path manifest = getManifestPath(packageName, packedMode);
    if (!fs::exists(manifest))
    {
        LOG("未查找到资源清单文件，请检查: {}", manifest.string());
        return false;
    }

    // packageOnly 模式下必须处于打包状态
    if (resourceInfo.packageOnly && !packedMode)
    {
        LOG("错误：资源包模式要求启用打包，但当前未处于打包状态");
        return false;
    }

    if (!packedMode)
    {
        // 自动打包：将 _manifest.txt 打包为 .ocdata + _packagemanifest.txt
        auto manifestFsPath = getManifestPath(packageName, false);
        bool result = generatePackage(manifestFsPath, false);
        if (!result)
        {
            LOG("自动打包失败，源清单文件: {}", manifestFsPath.string());
            return false;
        }

        packedMode = true;
        // 清除注册阶段的旧节点（startIndex/endIndex 均为 0），
        // 后续 findNode 将从 _packagemanifest.txt 加载正确偏移
        resourceManifestBuffer.clear();
        LOG("自动打包并启用资源包成功");
    }

    // 获取渲染器指针（纹理创建需要）
    renderer_ = OpenCoreManagers::GFXManager.getRenderer();
    if (!renderer_)
    {
        LOG("警告：获取渲染器失败，纹理加载将不可用");
    }

    return true;
}

// ──────────────────────────────────────────────
//  findNode — 按名称查找 ResourceNode
// ──────────────────────────────────────────────
ResourceNode *PackageManager::findNode(string_view name)
{
    // 1) 先查内存缓冲
    for (auto &entry : resourceManifestBuffer)
    {
        if (entry.name == name)
            return &entry;
    }

    // 2) 缓冲未命中 → 从清单位文件按需加载
    auto manifestFile = getManifestPath(packageName, packedMode);
    fstream manifest(manifestFile, ios::in | ios::binary);
    if (!manifest.is_open())
        return nullptr;

    string line;
    while (getline(manifest, line))
    {
        if (line.empty())
            continue;

        ResourceNode node = ResourceNode::deserialize(line);
        if (node.name == name)
        {
            // 加载到缓冲，下次直接命中
            resourceManifestBuffer.push_back(std::move(node));
            return &resourceManifestBuffer.back();
        }
    }
    return nullptr;
}

// ──────────────────────────────────────────────
//  extractResourceData — 从 .ocdata 提取原始二进制
// ──────────────────────────────────────────────
std::vector<char> PackageManager::extractResourceData(const ResourceNode &node)
{
    string ocdataPath = string("data//") + packageName + string("_00.ocdata");
    fstream file(
        fs::path(reinterpret_cast<const char8_t *>(ocdataPath.c_str())),
        ios::in | ios::binary);
    if (!file.is_open())
    {
        LOG("无法打开资源包文件 {}", ocdataPath);
        return {};
    }

    size_t dataSize = node.endIndex - node.startIndex;
    if (dataSize == 0)
        return {};

    std::vector<char> buf(dataSize);

    // 跳过 4 字节 OCDT 魔数 + startIndex
    file.seekg(4 + node.startIndex, ios::beg);
    if (!file.good())
    {
        LOG("资源包文件定位失败 offset={}", 4 + node.startIndex);
        return {};
    }

    file.read(buf.data(), dataSize);
    if (!file.good())
    {
        LOG("资源包文件读取失败 size={}", dataSize);
        return {};
    }

    return buf;
}

// ──────────────────────────────────────────────
//  requestTextureLoad — 异步加载纹理，返回 future 供去重
// ──────────────────────────────────────────────
std::shared_future<void> PackageManager::requestTextureLoad(string_view name)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto fut = promise->get_future().share();

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        pendingTextures_[string(name)] = fut;
    }

    // 在后台线程解码图像
    ThreadManager::getInstance().submit(
        [this, name = string(name), promise = std::move(promise)]
        {
            // --- 工作线程：解码 surface ---
            ResourceNode *node = findNode(name);
            if (!node)
            {
                LOG("资源 \"{}\" 未在清单中注册", name);
                promise->set_value();
                return;
            }

            auto data = extractResourceData(*node);
            if (data.empty())
            {
                promise->set_value();
                return;
            }

            SDL_IOStream *io = SDL_IOFromConstMem(data.data(), data.size());
            if (!io)
            {
                LOG("SDL_IOFromConstMem 失败");
                promise->set_value();
                return;
            }

            SDL_Surface *surface = IMG_Load_IO(io, true);
            if (!surface)
            {
                LOG("IMG_Load_IO 加载纹理 \"{}\" 失败: {}", name,
                    SDL_GetError());
                promise->set_value();
                return;
            }

            // 转换为标准格式（不涉及渲染器，工作线程安全）
            SDL_Surface *converted =
                SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
            SDL_DestroySurface(surface);

            if (!converted)
            {
                LOG("SDL_ConvertSurface 失败: {}", SDL_GetError());
                promise->set_value();
                return;
            }

            // 纹理创建必须在主线程（涉及渲染器）
            ThreadManager::getInstance().submit_to_main_thread(
                [this, name, converted, promise]
                {
                    // 若进入此 lambda 时 renderer_ 仍为空，记录警告
                    auto tex = ConvertSurfaceToTexture(renderer_, converted);
                    if (!tex)
                    {
                        LOG("ConvertSurfaceToTexture 失败，"
                            "renderer_=%p",
                            (void *)renderer_);
                    }

                    {
                        std::lock_guard<std::mutex> lock(cacheMutex_);
                        if (tex)
                            textureCache_[name] = std::move(tex);
                        pendingTextures_.erase(name);
                    }
                    promise->set_value();
                });
        });

    return fut;
}

// ──────────────────────────────────────────────
//  requestFontLoad — 异步加载字体，返回 future 供去重
// ──────────────────────────────────────────────
std::shared_future<void> PackageManager::requestFontLoad(string_view name,
                                                         int ptsize)
{
    string key = string(name) + "@" + std::to_string(ptsize);

    auto promise = std::make_shared<std::promise<void>>();
    auto fut = promise->get_future().share();

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        pendingFonts_[key] = fut;
    }

    // TTF_OpenFontIO 不涉及渲染器，工作线程完成即可
    ThreadManager::getInstance().submit(
        [this, name = string(name), key, ptsize, promise = std::move(promise)]
        {
            ResourceNode *node = findNode(name);
            if (!node)
            {
                LOG("字体 \"{}\" 未在清单中注册", name);
                promise->set_value();
                return;
            }

            auto data = extractResourceData(*node);
            if (data.empty())
            {
                promise->set_value();
                return;
            }

            SDL_IOStream *io = SDL_IOFromConstMem(data.data(), data.size());
            if (!io)
            {
                promise->set_value();
                return;
            }

            TTF_Font *font =
                TTF_OpenFontIO(io, true, static_cast<float>(ptsize));
            if (!font)
            {
                LOG("TTF_OpenFont_IO 加载字体 \"{}\" 失败: {}", name,
                    SDL_GetError());
                promise->set_value();
                return;
            }

            // TTF_Font 可能引用 IO 流的内存，data 必须和字体同寿命
            auto dataKeepAlive =
                std::make_shared<std::vector<char>>(std::move(data));
            auto fontPtr = shared_ptr<TTF_Font>(font,
                                                [dataKeepAlive](TTF_Font *f)
                                                {
                                                    (void)dataKeepAlive;
                                                    TextureDeleter{}(f);
                                                });

            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                fontCache_[key] = std::move(fontPtr);
                pendingFonts_.erase(key);
            }
            promise->set_value();
        });

    return fut;
}

// ──────────────────────────────────────────────
//  getTexture — 非阻塞资源获取（缓存 → 触发异步加载 → 返回 nullptr）
// ──────────────────────────────────────────────
shared_ptr<SDL_Texture> PackageManager::getTexture(string_view name)
{
    string key(name);
    bool needLoad = false;

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        // 阶段 1：缓存命中
        {
            auto it = textureCache_.find(key);
            if (it != textureCache_.end())
                return it->second;
        }

        // 阶段 2：已在加载中，不重复提交，不等待
        {
            auto it = pendingTextures_.find(key);
            if (it == pendingTextures_.end())
                needLoad = true;
        }
    }

    // 阶段 3：首次请求，释放锁后触发异步加载但不等待
    if (needLoad)
        requestTextureLoad(name);

    return nullptr;
}

// ──────────────────────────────────────────────
//  getTextureAsync — 同步阻塞加载纹理
// ──────────────────────────────────────────────
shared_ptr<SDL_Texture> PackageManager::getTextureAsync(string_view name)
{
    string key(name);

    // 阶段 1：缓存命中
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = textureCache_.find(key);
        if (it != textureCache_.end())
            return it->second;
    }

    // 阶段 2：已有异步加载在进行中，等待完成
    {
        std::unique_lock<std::mutex> lock(cacheMutex_);
        auto it = pendingTextures_.find(key);
        if (it != pendingTextures_.end())
        {
            auto fut = it->second;
            lock.unlock();
            fut.wait();
            lock.lock();
            auto cached = textureCache_.find(key);
            return cached != textureCache_.end() ? cached->second : nullptr;
        }
    }

    // 阶段 3：首次请求，当前线程同步完成全部加载
    ResourceNode *node = findNode(name);
    if (!node)
    {
        LOG("getTextureAsync: 资源 \"{}\" 未在清单中注册", name);
        return nullptr;
    }

    auto data = extractResourceData(*node);
    if (data.empty())
        return nullptr;

    SDL_IOStream *io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io)
    {
        LOG("getTextureAsync: SDL_IOFromConstMem 失败");
        return nullptr;
    }

    SDL_Surface *surface = IMG_Load_IO(io, true);
    if (!surface)
    {
        LOG("getTextureAsync: IMG_Load_IO 加载 \"{}\" 失败: {}", name,
            SDL_GetError());
        return nullptr;
    }

    SDL_Surface *converted =
        SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surface);
    if (!converted)
    {
        LOG("getTextureAsync: SDL_ConvertSurface 失败: {}", SDL_GetError());
        return nullptr;
    }

    auto tex = ConvertSurfaceToTexture(renderer_, converted);
    if (!tex)
    {
        LOG("getTextureAsync: ConvertSurfaceToTexture 失败");
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        textureCache_[key] = tex;
    }

    LOG("getTextureAsync: 同步加载完成 \"{}\"", name);
    return tex;
}

// ──────────────────────────────────────────────
//  getFont — 两阶段资源获取
// ──────────────────────────────────────────────
shared_ptr<TTF_Font> PackageManager::getFont(string_view name, int ptsize)
{
    string key = string(name) + "@" + std::to_string(ptsize);

    std::unique_lock<std::mutex> lock(cacheMutex_);

    // 阶段 1：缓存命中
    {
        auto it = fontCache_.find(key);
        if (it != fontCache_.end())
            return it->second;
    }

    // 阶段 2：已在加载中
    {
        auto it = pendingFonts_.find(key);
        if (it != pendingFonts_.end())
        {
            auto fut = it->second;
            lock.unlock();
            fut.wait();
            lock.lock();
            auto cached = fontCache_.find(key);
            return cached != fontCache_.end() ? cached->second : nullptr;
        }
    }

    // 阶段 3：首次请求，启动加载
    lock.unlock();
    auto fut = requestFontLoad(name, ptsize);
    fut.wait();

    lock.lock();
    auto cached = fontCache_.find(key);
    return cached != fontCache_.end() ? cached->second : nullptr;
}

// ──────────────────────────────────────────────
//  clearCache — 清空所有资源缓存
// ──────────────────────────────────────────────
void PackageManager::clearCache()
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    textureCache_.clear();
    fontCache_.clear();
}