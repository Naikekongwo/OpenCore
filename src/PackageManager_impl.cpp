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

    resource.rType    = rType;
    resource.name     = name;
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

    auto    packageManifestFile = getManifestPath(packageName, false);
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
    if (!timer || resourceManifestBuffer.empty())
        return;

    float now = timer->getTotalTime();

    // 末尾是 expireTime 最小的（最老），从后往前删
    while (!resourceManifestBuffer.empty() &&
           now > resourceManifestBuffer.back().expireTime)
    {
        resourceManifestBuffer.pop_back();
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
                entry.expireTime = timer->getTotalTime() + 15.0f;
            // 按 expireTime 降序排序，最近使用的排前面
            sort(resourceManifestBuffer.begin(), resourceManifestBuffer.end(),
                 [](const ResourceNode &a, const ResourceNode &b)
                 { return a.expireTime > b.expireTime; });
            return true;
        }
    }

    // 缓冲中未命中，尝试从清单文件中按需加载
    auto    packageManifestFile = getManifestPath(packageName, packedMode);
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
    string               line;
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
    ostringstream        resourceStream;
    size_t               currentOffset = 0;

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
        exportNode.name  = node.name;
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
    auto    packedManifestPath = getManifestPath(pkgName, true);
    fstream mfOut(packedManifestPath, ios::out | ios::binary | ios::trunc);
    if (!mfOut.is_open())
    {
        LOG("无法创建打包清单文件 {}", packedManifestPath.string());
        return false;
    }
    mfOut.write(packedManifestStr.data(), packedManifestStr.size());
    mfOut.close();

    // ⑥ 组装 .ocdata 文件（仅 OCDT 魔数 + 资源数据）
    auto    outputPath = fs::path(reinterpret_cast<const char8_t *>(
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
        bool result         = generatePackage(manifestFsPath, false);
        if (!result)
        {
            LOG("自动打包失败，源清单文件: {}", manifestFsPath.string());
            return false;
        }

        packedMode = true;
        LOG("自动打包并启用资源包成功");
    }
    return true;
}