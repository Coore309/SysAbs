// test01_file_attributes.cpp
// 全量测试 IFileSystem 接口的“文件存在性及其元数据查询”模块
// 测试目标：IFileSystem 第一模块（文件存在性及其元数据查询）接口（通过 WindowsFileSystem 实现）
// 测试目录：test\testzone

/*
该文件（test01_file_attributes.cpp）的所有代码均由 AI 生成。
只是我懒得写那么多而已，写库就已经够我掉几十根头发了。
*/

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <direct.h>   // _mkdir

// 设置控制台编码为 UTF-8
void SetConsoleUTF8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// 包含接口和实现（暂时直接包含 cpp，工厂模式之后再改）
#include <ifilesystem.h>
#include <windows_filesystem.h>

// ========== 辅助函数 ==========
bool create_test_file(const std::string& path, const std::string& content = "Hello") {
    std::ofstream file(path);
    if (file.is_open()) {
        file << content;
        file.close();
        return true;
    }
    return false;
}

bool create_test_dir(const std::string& path) {
    return CreateDirectoryA(path.c_str(), nullptr) != 0;
}

bool delete_test_file(const std::string& path) {
    return DeleteFileA(path.c_str()) != 0;
}

bool delete_test_dir(const std::string& path) {
    return RemoveDirectoryA(path.c_str()) != 0;
}

bool file_exists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool dir_exists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

void ensure_test_dir() {
    if (!dir_exists("test")) {
        _mkdir("test");
    }
    if (!dir_exists("test\\testzone")) {
        _mkdir("test\\testzone");
    }
}

// ========== 测试函数 ==========

bool test_exists(IFileSystem* fs) {
    std::cout << "\n--- test_exists ---" << std::endl;
    bool all_ok = true;

    // 1. 正常文件存在
    {
        std::string path = "test\\testzone\\normal_file.txt";
        create_test_file(path);
        auto err = fs->exists(path);
        bool ok = (err == FileSystemError::Success);
        std::cout << "  正常文件存在: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_file(path);
    }

    // 2. 目录存在
    {
        std::string path = "test\\testzone\\empty_dir";
        create_test_dir(path);
        auto err = fs->exists(path);
        bool ok = (err == FileSystemError::Success);
        std::cout << "  目录存在: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_dir(path);
    }

    // 3. 路径不存在
    {
        std::string path = "test\\testzone\\not_exist.txt";
        auto err = fs->exists(path);
        bool ok = (err == FileSystemError::NotFound);
        std::cout << "  路径不存在: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 4. 无效路径（含非法字符）
    {
        std::string path = "test\\testzone\\:invalid";
        auto err = fs->exists(path);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  无效路径（非法字符）: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 5. 空路径
    {
        std::string path = "";
        auto err = fs->exists(path);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  空路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 6. 权限受限的文件（需要手动准备）
    {
        const std::string path = "test\\testzone\\restricted_file.txt";
        if (!file_exists(path)) {
            std::cout << "  ⚠️ 受限文件不存在，请手动创建后按任意键继续..." << std::endl;
            system("pause");
        }
        if (file_exists(path)) {
            auto err = fs->exists(path);
            std::cout << "  err value: " << static_cast<int>(err) << std::endl;
            std::cout << "  错误信息: " << fs->getLastErrorMessage() << std::endl;
            bool ok = (err == FileSystemError::AccessDenied);
            std::cout << "  权限受限的文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
            if (!ok) all_ok = false;
        }
        else {
            std::cout << "  权限受限的文件: ⚠️ SKIP (文件不存在)" << std::endl;
        }
    }

    return all_ok;
}

bool test_getFileType(IFileSystem* fs) {
    std::cout << "\n--- test_getFileType ---" << std::endl;
    bool all_ok = true;

    // 1. 正常文件
    {
        std::string path = "test\\testzone\\normal_file.txt";
        create_test_file(path);
        FileType type;
        auto err = fs->getFileType(path, type);
        bool ok = (err == FileSystemError::Success && type == FileType::File);
        std::cout << "  正常文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_file(path);
    }

    // 2. 目录
    {
        std::string path = "test\\testzone\\empty_dir";
        create_test_dir(path);
        FileType type;
        auto err = fs->getFileType(path, type);
        bool ok = (err == FileSystemError::Success && type == FileType::Directory);
        std::cout << "  目录: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_dir(path);
    }

    // 3. 路径不存在
    {
        std::string path = "test\\testzone\\not_exist.txt";
        FileType type;
        auto err = fs->getFileType(path, type);
        bool ok = (err == FileSystemError::NotFound && type == FileType::NotFound);
        std::cout << "  路径不存在: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 4. 无效路径
    {
        std::string path = "test\\testzone\\:invalid";
        FileType type;
        auto err = fs->getFileType(path, type);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  无效路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 5. 空路径
    {
        std::string path = "";
        FileType type;
        auto err = fs->getFileType(path, type);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  空路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 6. 权限受限的文件
    {
        const std::string path = "test\\testzone\\restricted_file.txt";
        if (!file_exists(path)) {
            std::cout << "  ⚠️ 受限文件不存在，请手动创建后按任意键继续..." << std::endl;
            system("pause");
        }
        if (file_exists(path)) {
            FileType type;
            auto err = fs->getFileType(path, type);
            bool ok = (err == FileSystemError::AccessDenied);
            std::cout << "  权限受限的文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
            if (!ok) all_ok = false;
        }
        else {
            std::cout << "  权限受限的文件: ⚠️ SKIP" << std::endl;
        }
    }

    return all_ok;
}

bool test_getFileSize(IFileSystem* fs) {
    std::cout << "\n--- test_getFileSize ---" << std::endl;
    bool all_ok = true;

    // 1. 正常文件
    {
        std::string path = "test\\testzone\\normal_file.txt";
        const char* content = "Hello";
        create_test_file(path, content);
        uint64_t size;
        auto err = fs->getFileSize(path, size);
        bool ok = (err == FileSystemError::Success && size == strlen(content));
        std::cout << "  正常文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_file(path);
    }

    // 2. 目录（应返回 IsDirectory）
    {
        std::string path = "test\\testzone\\empty_dir";
        create_test_dir(path);
        uint64_t size;
        auto err = fs->getFileSize(path, size);
        bool ok = (err == FileSystemError::IsDirectory);
        std::cout << "  目录（应返回 IsDirectory）: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_dir(path);
    }

    // 3. 路径不存在
    {
        std::string path = "test\\testzone\\not_exist.txt";
        uint64_t size;
        auto err = fs->getFileSize(path, size);
        bool ok = (err == FileSystemError::NotFound);
        std::cout << "  路径不存在: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 4. 无效路径
    {
        std::string path = "test\\testzone\\:invalid";
        uint64_t size;
        auto err = fs->getFileSize(path, size);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  无效路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 5. 空路径
    {
        std::string path = "";
        uint64_t size;
        auto err = fs->getFileSize(path, size);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  空路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 6. 权限受限的文件
    {
        const std::string path = "test\\testzone\\restricted_file.txt";
        if (!file_exists(path)) {
            std::cout << "  ⚠️ 受限文件不存在，请手动创建后按任意键继续..." << std::endl;
            system("pause");
        }
        if (file_exists(path)) {
            uint64_t size;
            auto err = fs->getFileSize(path, size);
            bool ok = (err == FileSystemError::AccessDenied);
            std::cout << "  权限受限的文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
            if (!ok) all_ok = false;
        }
        else {
            std::cout << "  权限受限的文件: ⚠️ SKIP" << std::endl;
        }
    }

    return all_ok;
}

bool test_getLastModifiedTime(IFileSystem* fs) {
    std::cout << "\n--- test_getLastModifiedTime ---" << std::endl;
    bool all_ok = true;

    // 1. 正常文件
    {
        std::string path = "test\\testzone\\normal_file.txt";
        create_test_file(path, "Test");
        int64_t time;
        auto err = fs->getLastModifiedTime(path, time);
        bool ok = (err == FileSystemError::Success && time > 0);
        std::cout << "  正常文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_file(path);
    }

    // 2. 目录（应返回 IsDirectory）
    {
        std::string path = "test\\testzone\\empty_dir";
        create_test_dir(path);
        int64_t time;
        auto err = fs->getLastModifiedTime(path, time);
        bool ok = (err == FileSystemError::IsDirectory);
        std::cout << "  目录（应返回 IsDirectory）: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
        delete_test_dir(path);
    }

    // 3. 路径不存在
    {
        std::string path = "test\\testzone\\not_exist.txt";
        int64_t time;
        auto err = fs->getLastModifiedTime(path, time);
        bool ok = (err == FileSystemError::NotFound);
        std::cout << "  路径不存在: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 4. 无效路径
    {
        std::string path = "test\\testzone\\:invalid";
        int64_t time;
        auto err = fs->getLastModifiedTime(path, time);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  无效路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 5. 空路径
    {
        std::string path = "";
        int64_t time;
        auto err = fs->getLastModifiedTime(path, time);
        bool ok = (err == FileSystemError::InvalidPath);
        std::cout << "  空路径: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
        if (!ok) all_ok = false;
    }

    // 6. 权限受限的文件
    {
        const std::string path = "test\\testzone\\restricted_file.txt";
        if (!file_exists(path)) {
            std::cout << "  ⚠️ 受限文件不存在，请手动创建后按任意键继续..." << std::endl;
            system("pause");
        }
        if (file_exists(path)) {
            int64_t time;
            auto err = fs->getLastModifiedTime(path, time);
            bool ok = (err == FileSystemError::AccessDenied);
            std::cout << "  权限受限的文件: " << (ok ? "✅ PASS" : "❌ FAIL") << std::endl;
            if (!ok) all_ok = false;
        }
        else {
            std::cout << "  权限受限的文件: ⚠️ SKIP" << std::endl;
        }
    }

    return all_ok;
}

// ========== 主函数 ==========
int main() {
    SetConsoleUTF8();

    std::cout << "=== IFileSystem 文件元数据查询全量测试 ===" << std::endl;
    std::cout << "提示：测试前请确保 test\\testzone 目录存在" << std::endl;
    std::cout << "如需测试 AccessDenied，请按提示创建受限文件" << std::endl;

    ensure_test_dir();

    WindowsFileSystem fs;

    bool all_passed = true;
    all_passed &= test_exists(&fs);
    all_passed &= test_getFileType(&fs);
    all_passed &= test_getFileSize(&fs);
    all_passed &= test_getLastModifiedTime(&fs);

    std::cout << "\n=== 测试结果 ===" << std::endl;
    std::cout << (all_passed ? "🎉 所有测试通过！" : "❌ 部分测试失败，请检查输出。") << std::endl;

    system("pause");
    return all_passed ? 0 : 1;
}