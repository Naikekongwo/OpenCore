#include "Asset/PackageManager.hpp"
#include "OpenCore.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <initializer_list>
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

    string packageManifestFile;
    packageManifestFile =
        string("data//") + packageName + string("_manifest.txt");

    if (std::filesystem::exists(packageManifestFile))
    {
        std::filesystem::remove(packageManifestFile);
    }

    string package_header =
        string("data//") + packageName + string("_00.ocdata");
    if (std::filesystem::exists(package_header))
    {
        // 此举是因为发现了资源集
        // 因故将模式设置为packedMode
        packedMode = true;
    }

    if (packedMode)
    {
        // <TODO> 此处加入从压缩包中提取清单的工作
        packageManifestFile =
            string("data//") + packageName + string("_packagemanifest.txt");
        LOG("初始化成功，当前资源管理系统运行在PACKAGE模式，清单文件已经提取到 "
            "{}",
            packageManifestFile);
    }
    else
    {
        fstream newManifest;
        newManifest.open(packageManifestFile, std::ios::out);
        newManifest.close();

        LOG("初始化成功，清单文件已生成到 {}", packageManifestFile);
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

    if (contains(resource))
        return true;

    resourceManifestBuffer.push_back(resource);

    string packageManifestFile =
        string("data//") + packageName + string("_manifest.txt");

    fstream manifest(packageManifestFile.c_str(), std::ios::app);

    if (!manifest.good())
    {
        LOG("发生了意外的错误，该清单文件不存在 {}", packageManifestFile);
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

bool PackageManager::contains(ResourceNode target)
{
    bool found = false;
    for (const auto &entry : resourceManifestBuffer)
    {
        if (entry == target)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        string packageManifestFile =
            string("data//") + packageName + string("_manifest.txt");
        fstream manifest(packageManifestFile.c_str(), std::ios::in);
        if (!manifest.good())
        {
            // 文件不存在，创建新的空清单文件
            manifest.open(packageManifestFile, std::ios::out);
            manifest.close();
            return false;
        }

        string line;
        while (std::getline(manifest, line))
        {
            if (!line.empty())
            {
                ResourceNode node = ResourceNode::deserialize(line);
                if (node == target)
                {
                    // 存在于清单文件但在内存中不存在（超时淘汰后重新找回）
                    resourceManifestBuffer.push_back(target);
                    return true;
                }
            }
        }
        manifest.close();
    }

    return found;
}

bool PackageManager::extractManifest(string_view packagePath)
{
    // <TODO> 提取packagemanifest
    // 文件结构：[OCCP][packagemanifest内容][资源数据][4字节manifest长度]
    //
    // ①提取文件末端4字节（uint32_t），得到manifest内容的字节长度
    // ②从第4字节（跳过4字节魔数）开始，读取manifestLength字节
    //    即得到完整的packagemanifest内容
    // ③从packagePath文件名中提取packageName
    //    命名格式为 xxxx_xx.ocdata，取第一个下划线之前的部分
    // ④将提取的内容写入 data//xxxx_manifest.txt
    // ⑤返回提取结果
    return true;
}

bool PackageManager::generatePackage(string_view manifestPath)
{
    // <TODO> 生成资源包文件
    // 最终文件结构：[OCCP][packagemanifest][资源数据][4字节manifest长度]
    //
    // ①逐行遍历manifest，检查每个资源文件是否存在
    //    若有文件不存在，LOG后返回false
    // ②全部存在时开始生成资源包：
    // ③逐行遍历manifest，生成新导出节点：
    //    Path置空，startIndex/endIndex根据文件大小累进计算
    //    startIndex = 当前资源在资源数据区的起始偏移（从0开始）
    //    endIndex   = startIndex + 文件大小
    // ④将新节点逐行写入 xxxx_packagemanifest.txt
    //    同时将manifest中每个资源文件内容拼接到资源数据缓冲区
    // ⑤计算 packagemanifest 的字节长度，作为末尾4字节的值
    // ⑥按以下顺序组装最终文件：
    //    a. 写入魔数 "OCCP"
    //    b. 写入 packagemanifest 内容
    //    c. 写入资源数据缓冲区
    //    d. 末尾追加4字节（packagemanifest的字节长度，uint32_t）
    return true;
}

bool PackageManager::enablePackage(string_view packagePath)
{
    // <TODO> 启用已安装的资源包
    // 文件结构：[OCCP][packagemanifest][资源数据][4字节manifest长度]
    //
    // ①验证文件头部4字节魔数是否为 "OCCP"
    // ②调用extractManifest提取packagemanifest到 data//xxxx_manifest.txt
    // ③将魔数 "OCCP" 改为 "DATA"（标记为已启用）
    // ④删除packagemanifest内容（移除 [packagemanifest] 段）
    //   并将资源数据前移，覆盖原manifest位置
    // ⑤移除末尾4字节（manifest长度信息不再需要）
    //   最终文件结构：[DATA][资源数据]
    return true;
}