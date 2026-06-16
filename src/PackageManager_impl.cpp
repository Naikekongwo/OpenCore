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

using std::fstream;

PackageManager::PackageManager(string_view pName)
{
    if (pName.empty())
    {
        LOG("初始化包名称遇到空值");
        throw std::runtime_error("packagename is null.");
    }

    if (!std::filesystem::exists("data//"))
    {
        std::filesystem::create_directory("data//");
    }

    packageName = pName;

    // 清除旧的非打包模式清单文件
    auto packageManifestFile = getManifestPath(packageName, false);
    if (std::filesystem::exists(packageManifestFile))
    {
        std::filesystem::remove(packageManifestFile);
    }

    auto packageOCData = std::filesystem::path(
        string("data//") + string(packageName) + string("_00.ocdata"));
    if (std::filesystem::exists(packageOCData))
    {
        packedMode = true;

        // 读取魔数判断资源包状态
        char magic[4]{};

        fstream pkgFile(packageOCData, std::ios::in | std::ios::binary);
        if (pkgFile.is_open())
        {
            pkgFile.read(magic, 4);
            pkgFile.close();
        }

        auto packedManifest = getManifestPath(packageName, true);

        if (std::strncmp(magic, "OCCP", 4) == 0)
        {
            extractManifest(packageOCData.string());
            LOG("初始化成功，已从资源包提取清单到 {}", packedManifest.string());
        }
        else if (std::strncmp(magic, "DATA", 4) == 0)
        {
            if (!std::filesystem::exists(packedManifest))
            {
                LOG("错误：资源包已启用但本地找不到清单文件 {}",
                    packedManifest.string());
            }
            else
            {
                LOG("初始化成功，已加载已启用的资源包清单 {}",
                    packedManifest.string());
            }
        }
        else
        {
            LOG("错误：未知的资源包魔数，文件可能已损坏");
        }
    }
    else
    {
        fstream newManifest;
        newManifest.open(packageManifestFile, std::ios::out);
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
    fstream manifest(packageManifestFile, std::ios::app | std::ios::binary);

    if (!manifest.good())
    {
        LOG("发生了意外的错误，该清单文件不存在 {}",
            packageManifestFile.string());
        return false;
    }

    manifest << resource.serialize() << std::endl;

    manifest.close();
    return true;
}

bool PackageManager::registerResources(
    std::initializer_list<ResourceNode> resources)
{
    bool result = true;
    for (const auto &entry : resources)
    {
        result = result && registerResource(entry);
    }
    return result;
}

std::filesystem::path PackageManager::getManifestPath(string_view packageName,
                                                      bool        packed)
{
    string path = string("data//") + string(packageName) +
                  (packed ? "_packagemanifest.txt" : "_manifest.txt");
    return std::filesystem::path(path);
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
            std::sort(resourceManifestBuffer.begin(),
                      resourceManifestBuffer.end(),
                      [](const ResourceNode &a, const ResourceNode &b)
                      { return a.expireTime > b.expireTime; });
            return true;
        }
    }

    // 缓冲中未命中，尝试从清单文件中按需加载
    auto    packageManifestFile = getManifestPath(packageName, packedMode);
    fstream manifest(packageManifestFile, std::ios::in | std::ios::binary);
    if (manifest.good())
    {
        string line;
        while (std::getline(manifest, line))
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
                    std::sort(resourceManifestBuffer.begin(),
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

bool PackageManager::extractManifest(string_view packagePath)
{
    fstream pkgFile(std::filesystem::path(string(packagePath)),
                    std::ios::in | std::ios::binary);
    if (!pkgFile.is_open())
    {
        LOG("无法打开资源包文件 {}", packagePath);
        return false;
    }

    // 获取文件大小
    pkgFile.seekg(0, std::ios::end);
    auto fileSize = pkgFile.tellg();
    if (fileSize < 8) // 至少需要 魔数(4) + 长度值(4)
    {
        LOG("资源包文件过小 {}", packagePath);
        return false;
    }

    // ① 读取末尾4字节 → manifest 长度
    pkgFile.seekg(-4, std::ios::end);
    uint32_t manifestLength = 0;
    pkgFile.read(reinterpret_cast<char *>(&manifestLength),
                 sizeof(manifestLength));

    if (manifestLength == 0 ||
        manifestLength > static_cast<uint32_t>(fileSize) - 8)
    {
        LOG("资源包清单长度无效 {}", manifestLength);
        return false;
    }

    // ② 从第4字节（跳过魔数）开始读取 manifest 内容
    pkgFile.seekg(4, std::ios::beg);
    string manifestContent(manifestLength, '\0');
    pkgFile.read(manifestContent.data(), manifestLength);

    auto bytesRead = pkgFile.gcount();
    pkgFile.close();

    if (bytesRead != static_cast<std::streamsize>(manifestLength))
    {
        LOG("读取资源包清单内容不完整");
        return false;
    }

    // ③ 从 packagePath 中提取包名 xxxx_xx.ocdata → xxxx
    string pathStr(packagePath);
    auto   slashPos = pathStr.find_last_of("/\\");
    string fileName =
        (slashPos == string::npos) ? pathStr : pathStr.substr(slashPos + 1);
    auto   underscorePos = fileName.find('_');
    string pkgName       = (underscorePos == string::npos)
                               ? fileName.substr(0, fileName.find_last_of('.'))
                               : fileName.substr(0, underscorePos);

    // ④ 写入 data//xxxx_packagemanifest.txt
    auto    destPath = getManifestPath(pkgName, true);
    fstream dest(destPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!dest.is_open())
    {
        LOG("无法创建清单文件 {}", destPath.string());
        return false;
    }
    dest.write(manifestContent.data(), manifestContent.size());
    dest.close();

    LOG("清单已从资源包提取到 {}", destPath.string());
    return true;
}

bool PackageManager::generatePackage(string_view manifestPath, bool cleanup)
{
    // 读取原始清单
    fstream manifestFile(string(manifestPath), std::ios::in | std::ios::binary);
    if (!manifestFile.is_open())
    {
        LOG("无法打开清单文件 {}", manifestPath);
        return false;
    }

    vector<ResourceNode> originalNodes;
    string               line;
    while (std::getline(manifestFile, line))
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
        if (!std::filesystem::exists(node.filePath))
        {
            LOG("资源文件不存在 {}", node.filePath);
            return false;
        }
    }

    // ③④ 遍历manifest，生成导出节点并拼接资源数据
    vector<ResourceNode> exportNodes;
    std::ostringstream   resourceStream;
    size_t               currentOffset = 0;

    for (const auto &node : originalNodes)
    {
        fstream resFile(std::filesystem::path(node.filePath),
                        std::ios::in | std::ios::binary);
        if (!resFile.is_open())
        {
            LOG("无法读取资源文件 {}", node.filePath);
            return false;
        }

        string content((std::istreambuf_iterator<char>(resFile)),
                       std::istreambuf_iterator<char>());
        resFile.close();

        ResourceNode exportNode;
        exportNode.rType      = node.rType;
        exportNode.name       = node.name;
        exportNode.filePath   = ""; // 打包后路径为空
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

    // 从 manifestPath 提取包名
    string pathStr(manifestPath);
    auto   slashPos = pathStr.find_last_of("/\\");
    string fileName =
        (slashPos == string::npos) ? pathStr : pathStr.substr(slashPos + 1);
    auto   underscorePos = fileName.find('_');
    string pkgName       = (underscorePos == string::npos)
                               ? fileName.substr(0, fileName.find_last_of('.'))
                               : fileName.substr(0, underscorePos);

    // ⑤ 计算 manifest 长度
    uint32_t manifestLength = static_cast<uint32_t>(packedManifestStr.size());

    // ⑥ 组装最终文件
    auto    outputPath = std::filesystem::path(string("data//") + pkgName +
                                               string("_00.ocdata"));
    fstream output(outputPath,
                   std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        LOG("无法创建资源包文件 {}", outputPath.string());
        return false;
    }

    output.write("OCCP", 4); // a. 魔数
    output.write(packedManifestStr.data(),
                 packedManifestStr.size()); // b. packagemanifest
    output.write(resourceData.data(), resourceData.size()); // c. 资源数据
    output.write(reinterpret_cast<const char *>(&manifestLength),
                 sizeof(manifestLength)); // d. 末尾4字节

    output.close();

    LOG("资源包已生成到 {}", outputPath.string());

    if (cleanup)
    {
        // 清理原始资源文件和清单
        std::error_code ec;
        for (const auto &node : originalNodes)
        {
            std::filesystem::remove(std::filesystem::path(node.filePath), ec);
            if (ec)
                LOG("警告：无法删除源文件 {} - {}", node.filePath,
                    ec.message());
        }
        std::filesystem::remove(std::filesystem::path(manifestPath), ec);
        if (ec)
            LOG("警告：无法删除原始清单 {} - {}", manifestPath, ec.message());
    }

    return true;
}

bool PackageManager::enablePackage(string_view packagePath)
{
    fstream pkgFile(std::filesystem::path(string(packagePath)),
                    std::ios::in | std::ios::binary);
    if (!pkgFile.is_open())
    {
        LOG("无法打开资源包文件 {}", packagePath);
        return false;
    }

    // ① 验证魔数
    char magic[4]{};
    pkgFile.read(magic, 4);
    if (std::strncmp(magic, "OCCP", 4) != 0)
    {
        LOG("资源包魔数不正确（期望 OCCP，当前为 %.4s），无法启用", magic);
        return false;
    }

    // ② 先提取清单
    if (!extractManifest(packagePath))
    {
        LOG("提取清单失败，启用流程终止");
        return false;
    }

    // 读取文件尾4字节 → manifest 长度
    pkgFile.seekg(0, std::ios::end);
    auto fileSize = pkgFile.tellg();
    pkgFile.seekg(-4, std::ios::end);
    uint32_t manifestLength = 0;
    pkgFile.read(reinterpret_cast<char *>(&manifestLength),
                 sizeof(manifestLength));

    if (manifestLength == 0 ||
        manifestLength > static_cast<uint32_t>(fileSize) - 8)
    {
        LOG("清单长度无效，无法启用");
        return false;
    }

    // 计算资源数据区
    // 文件结构：[OCCP(4)][manifest(manifestLength)][资源数据][4字节]
    size_t dataOffset = 4 + manifestLength;
    size_t dataSize   = static_cast<size_t>(fileSize) - 4 - manifestLength - 4;

    // 读取资源数据
    vector<char> resourceData(dataSize);
    pkgFile.seekg(static_cast<std::streamoff>(dataOffset), std::ios::beg);
    pkgFile.read(resourceData.data(), dataSize);
    pkgFile.close();

    // 重写文件：[DATA][资源数据]
    fstream outFile(string(packagePath),
                    std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        LOG("无法重写资源包文件 {}", packagePath);
        return false;
    }

    // ③ 写入 DATA 魔数
    outFile.write("DATA", 4);
    // ④ 写入资源数据（manifest 段已移除）
    outFile.write(resourceData.data(), resourceData.size());
    // ⑤ 末尾4字节自然移除（不写入）
    outFile.close();

    LOG("资源包已启用：{}", packagePath);
    return true;
}