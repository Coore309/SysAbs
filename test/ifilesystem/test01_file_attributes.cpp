#if 0
// ============================================================================
// test01_file_attributes.cpp
// SysAbs 第一模块全量测试 - 文件存在性及其元数据查询
// 编译输出目录: $(ProjectDir)test
// 工作目录: $(ProjectDir)test\testzone
// 路径基准: 程序所在目录（exe位置）
// 操作走接口，验证走底层 WinAPI（独立检验结果正确性）
// ============================================================================

/*
该文件的所有代码均由 AI 生成。
*/

#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#include <sysabs/ifilesystem.h>
#include <Windows.h>

using namespace sysabs;

// ---------------------------------------------------------------------------
// 辅助工具（WinAPI，用于测试环境准备、验证、清理）
// ---------------------------------------------------------------------------

static std::wstring testChartoWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
    wstr.pop_back();
    return wstr;
}

static std::string getExeDirectory() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring wpath(exePath);
    size_t pos = wpath.find_last_of(L"\\");
    if (pos != std::wstring::npos) wpath = wpath.substr(0, pos + 1);
    int len = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &result[0], len, nullptr, nullptr);
    result.pop_back();
    return result;
}

static bool createDirWin(const std::wstring& wpath) {
    return CreateDirectoryW(wpath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool createFileWin(const std::wstring& wpath, const char* content) {
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(h, content, static_cast<DWORD>(strlen(content)), &written, NULL);
    CloseHandle(h);
    return ok;
}

// 用 WinAPI 获取路径属性数据（用于验证）
static bool winGetAttrData(const std::wstring& wpath, _WIN32_FILE_ATTRIBUTE_DATA& out) {
    return GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &out) != 0;
}

// FILETIME → 毫秒时间戳（与接口 getLastModifiedTime 的换算一致）
static int64_t filetimeToMillis(FILETIME ft) {
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return static_cast<int64_t>((u.QuadPart - 116444736000000000ULL) / 10000);
}

static void deleteDirRecursive(const std::wstring& wdir) {
    std::wstring search = wdir + L"\\*";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;
        std::wstring fullPath = wdir + L"\\" + ffd.cFileName;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            deleteDirRecursive(fullPath);
        }
        else {
            SetFileAttributesW(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(fullPath.c_str());
        }
    } while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);
    RemoveDirectoryW(wdir.c_str());
}

static void testPass(const char* name) { std::cout << "[PASS] " << name << std::endl; }
static void testFail(const char* name, const char* detail) { std::cout << "[FAIL] " << name << " : " << detail << std::endl; }

// ---------------------------------------------------------------------------
// 测试主逻辑
// ---------------------------------------------------------------------------

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "========================================" << std::endl;
    std::cout << " SysAbs 第一模块全量测试 - 元数据查询" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string exeDir = getExeDirectory();
    std::string zoneDir = exeDir + "testzone\\";
    std::wstring wZoneDir = testChartoWide(zoneDir);
    std::wstring wTestDir = wZoneDir + L"test_first_module";

    if (!createDirWin(wTestDir)) {
        std::cout << "[ERROR] 无法创建测试目录" << std::endl;
        return 1;
    }
    std::cout << "[环境] 测试目录: " << zoneDir << "test_first_module" << std::endl;

    auto fs = createFileSystem();
    if (!fs) { std::cout << "[ERROR] createFileSystem() 返回 nullptr" << std::endl; return 1; }
    std::cout << "[环境] IFileSystem 实例创建成功" << std::endl << std::endl;

    int totalTests = 0, passedTests = 0, failedTests = 0;
    auto report = [&](bool ok, const char* name, const char* detail = "") {
        totalTests++;
        if (ok) { passedTests++; testPass(name); }
        else { failedTests++; testFail(name, detail); }
        };

    auto wpath = [&](const char* rel) -> std::wstring { return wTestDir + L"\\" + testChartoWide(rel); };
    auto spath = [&](const char* rel) -> std::string { return zoneDir + "test_first_module\\" + rel; };

    std::string p1;
    FileType outType;
    uint64_t outSize = 0;
    int64_t outTime = 0;

    // =====================================================================
    // 1. validatePath 测试
    // =====================================================================
    std::cout << "--- validatePath 测试 ---" << std::endl;

    report(!fs->validatePath(""), "空路径 → false");
    report(fs->validatePath("C:/fish/nemo.txt"), "合法路径 → true");
    report(!fs->validatePath("D:/fish/na?me.txt"), "非法字符 ? → false");
    report(!fs->validatePath("1:/fish"), "非法盘符 → false");
    report(fs->validatePath("c:/fish"), "小写盘符 → true");

    std::cout << std::endl;

    // =====================================================================
    // 2. exists 测试
    // =====================================================================
    std::cout << "--- exists 测试 ---" << std::endl;

    // 2.1 文件存在
    {
        p1 = spath("exists_file.txt");
        createFileWin(wpath("exists_file.txt"), "data");
        auto err = fs->exists(p1);
        report(err == FileSystemError::Success, "文件存在 → Success",
            err == FileSystemError::Success ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("exists_file.txt").c_str());
    }

    // 2.2 目录存在
    {
        p1 = spath("exists_dir");
        createDirWin(wpath("exists_dir"));
        auto err = fs->exists(p1);
        report(err == FileSystemError::Success, "目录存在 → Success",
            err == FileSystemError::Success ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("exists_dir"));
    }

    // 2.3 不存在
    {
        p1 = spath("exists_nonexistent");
        auto err = fs->exists(p1);
        report(err == FileSystemError::NotFound, "路径不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.4 空路径
    {
        auto err = fs->exists("");
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.5 非法字符路径
    {
        p1 = spath("illegal?.txt");
        auto err = fs->exists(p1);
        report(err == FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 3. getFileType 测试
    // =====================================================================
    std::cout << "--- getFileType 测试 ---" << std::endl;

    // 3.1 文件（WinAPI 验证确为文件）
    {
        p1 = spath("type_file.txt");
        createFileWin(wpath("type_file.txt"), "data");
        outType = FileType::Unknown;
        auto err = fs->getFileType(p1, outType);
        _WIN32_FILE_ATTRIBUTE_DATA attr;
        bool winIsFile = winGetAttrData(wpath("type_file.txt"), attr)
            && !(attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        bool ok = (err == FileSystemError::Success && outType == FileType::File && winIsFile);
        report(ok, "文件 → Success + File", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("type_file.txt").c_str());
    }

    // 3.2 目录（WinAPI 验证确为目录）
    {
        p1 = spath("type_dir");
        createDirWin(wpath("type_dir"));
        outType = FileType::Unknown;
        auto err = fs->getFileType(p1, outType);
        _WIN32_FILE_ATTRIBUTE_DATA attr;
        bool winIsDir = winGetAttrData(wpath("type_dir"), attr)
            && (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        bool ok = (err == FileSystemError::Success && outType == FileType::Directory && winIsDir);
        report(ok, "目录 → Success + Directory", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("type_dir"));
    }

    // 3.3 不存在
    {
        p1 = spath("type_nonexistent");
        outType = FileType::Unknown;
        auto err = fs->getFileType(p1, outType);
        report(err == FileSystemError::NotFound && outType == FileType::NotFound, "不存在 → NotFound + FileType::NotFound",
            err == FileSystemError::NotFound ? (outType == FileType::NotFound ? "" : "outType 不为 NotFound") : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.4 空路径
    {
        outType = FileType::Unknown;
        auto err = fs->getFileType("", outType);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.5 非法字符路径
    {
        p1 = spath("illegal|.txt");
        outType = FileType::Unknown;
        auto err = fs->getFileType(p1, outType);
        report(err == FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 4. getFileSize 测试
    // =====================================================================
    std::cout << "--- getFileSize 测试 ---" << std::endl;

    // 4.1 正常文件（WinAPI 验证大小一致）
    {
        p1 = spath("size_file.bin");
        const char* content = "1234567890";  // 10 字节
        createFileWin(wpath("size_file.bin"), content);
        outSize = 0;
        auto err = fs->getFileSize(p1, outSize);
        _WIN32_FILE_ATTRIBUTE_DATA attr;
        winGetAttrData(wpath("size_file.bin"), attr);
        uint64_t winSize = (static_cast<uint64_t>(attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;
        bool ok = (err == FileSystemError::Success && outSize == 10 && winSize == 10);
        report(ok, "正常文件 → Success + 大小正确", ok ? "" : ("err=" + std::to_string(static_cast<int>(err)) + " size=" + std::to_string(outSize)).c_str());
        DeleteFileW(wpath("size_file.bin").c_str());
    }

    // 4.2 空文件
    {
        p1 = spath("size_empty.txt");
        createFileWin(wpath("size_empty.txt"), "");
        outSize = 999;
        auto err = fs->getFileSize(p1, outSize);
        report(err == FileSystemError::Success && outSize == 0, "空文件 → Success + 0",
            err == FileSystemError::Success ? (outSize == 0 ? "" : "大小不为0") : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("size_empty.txt").c_str());
    }

    // 4.3 目录
    {
        p1 = spath("size_dir");
        createDirWin(wpath("size_dir"));
        outSize = 0;
        auto err = fs->getFileSize(p1, outSize);
        report(err == FileSystemError::IsDirectory, "目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("size_dir"));
    }

    // 4.4 不存在
    {
        p1 = spath("size_nonexistent");
        outSize = 0;
        auto err = fs->getFileSize(p1, outSize);
        report(err == FileSystemError::NotFound, "不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 4.5 空路径
    {
        outSize = 0;
        auto err = fs->getFileSize("", outSize);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 5. getLastModifiedTime 测试
    // =====================================================================
    std::cout << "--- getLastModifiedTime 测试 ---" << std::endl;

    // 5.1 正常文件（WinAPI 验证时间戳一致）
    {
        p1 = spath("time_file.txt");
        createFileWin(wpath("time_file.txt"), "data");
        outTime = 0;
        auto err = fs->getLastModifiedTime(p1, outTime);
        _WIN32_FILE_ATTRIBUTE_DATA attr;
        winGetAttrData(wpath("time_file.txt"), attr);
        int64_t winTime = filetimeToMillis(attr.ftLastWriteTime);
        bool ok = (err == FileSystemError::Success && outTime == winTime && outTime > 0);
        report(ok, "正常文件 → Success + 时间戳一致", ok ? "" : ("err=" + std::to_string(static_cast<int>(err)) + " time=" + std::to_string(outTime)).c_str());
        DeleteFileW(wpath("time_file.txt").c_str());
    }

    // 5.2 目录
    {
        p1 = spath("time_dir");
        createDirWin(wpath("time_dir"));
        outTime = 0;
        auto err = fs->getLastModifiedTime(p1, outTime);
        report(err == FileSystemError::IsDirectory, "目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("time_dir"));
    }

    // 5.3 不存在
    {
        p1 = spath("time_nonexistent");
        outTime = 0;
        auto err = fs->getLastModifiedTime(p1, outTime);
        report(err == FileSystemError::NotFound, "不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 5.4 空路径
    {
        outTime = 0;
        auto err = fs->getLastModifiedTime("", outTime);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 清理
    // =====================================================================
    std::cout << "--- 清理 ---" << std::endl;
    deleteDirRecursive(wTestDir);
    std::cout << "[清理] 已删除: " << zoneDir << "test_first_module" << std::endl << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << " 测试结果汇总" << std::endl;
    std::cout << " 总计: " << totalTests << std::endl;
    std::cout << " 通过: " << passedTests << std::endl;
    std::cout << " 失败: " << failedTests << std::endl;
    std::cout << "========================================" << std::endl;

    return failedTests > 0 ? 1 : 0;
}
#endif