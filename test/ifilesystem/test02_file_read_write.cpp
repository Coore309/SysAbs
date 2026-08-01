#if 0
// ============================================================================
// test02_file_read_write.cpp
// SysAbs 第二模块全量测试 - 文件二进制读写
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

static const char* RESTRICTED_FILE = "restricted_file.txt";

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

static bool createFileWin(const std::wstring& wpath, const void* data, DWORD size) {
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(h, data, size, &written, NULL) && written == size;
    CloseHandle(h);
    return ok;
}
static bool createFileWin(const std::wstring& wpath, const char* content) {
    return createFileWin(wpath, content, static_cast<DWORD>(strlen(content)));
}

// 用 WinAPI 读取文件全部内容（验证辅助）
static bool winReadFile(const std::wstring& wpath, std::vector<uint8_t>& out) {
    out.clear();
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size;
    if (!GetFileSizeEx(h, &size)) { CloseHandle(h); return false; }
    out.resize(static_cast<size_t>(size.QuadPart));
    if (size.QuadPart > 0) {
        DWORD read = 0;
        if (!ReadFile(h, out.data(), static_cast<DWORD>(size.QuadPart), &read, NULL) || read != size.QuadPart) {
            CloseHandle(h);
            out.clear();
            return false;
        }
    }
    CloseHandle(h);
    return true;
}

static bool winPathExists(const std::wstring& wpath) {
    return GetFileAttributesW(wpath.c_str()) != INVALID_FILE_ATTRIBUTES;
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
static void testSkip(const char* name, const char* reason) { std::cout << "[SKIP] " << name << " : " << reason << std::endl; }

// ---------------------------------------------------------------------------
// 测试主逻辑
// ---------------------------------------------------------------------------

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "========================================" << std::endl;
    std::cout << " SysAbs 第二模块全量测试 - 文件二进制读写" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string exeDir = getExeDirectory();
    std::string zoneDir = exeDir + "testzone\\";
    std::wstring wZoneDir = testChartoWide(zoneDir);
    std::wstring wRestricted = wZoneDir + L"restricted_file.txt";
    std::wstring wTestDir = wZoneDir + L"test_second_module";

    bool hasRestricted = winPathExists(wRestricted);
    std::cout << "[环境] restricted_file.txt: " << (hasRestricted ? "已找到" : "未找到") << std::endl;

    if (!createDirWin(wTestDir)) {
        std::cout << "[ERROR] 无法创建测试目录" << std::endl;
        return 1;
    }
    std::cout << "[环境] 测试目录: " << zoneDir << "test_second_module" << std::endl;

    auto fs = createFileSystem();
    if (!fs) { std::cout << "[ERROR] createFileSystem() 返回 nullptr" << std::endl; return 1; }
    std::cout << "[环境] IFileSystem 实例创建成功" << std::endl << std::endl;

    int totalTests = 0, passedTests = 0, failedTests = 0, skippedTests = 0;
    auto report = [&](bool ok, const char* name, const char* detail = "") {
        totalTests++;
        if (ok) { passedTests++; testPass(name); }
        else { failedTests++; testFail(name, detail); }
        };
    auto skip = [&](const char* name, const char* reason) {
        totalTests++; skippedTests++; testSkip(name, reason);
        };

    auto wpath = [&](const char* rel) -> std::wstring { return wTestDir + L"\\" + testChartoWide(rel); };
    auto spath = [&](const char* rel) -> std::string { return zoneDir + "test_second_module\\" + rel; };

    std::string p1;
    std::vector<uint8_t> data, verify;

    // =====================================================================
    // 1. readAllBytes 测试
    // =====================================================================
    std::cout << "--- readAllBytes 测试 ---" << std::endl;

    // 1.1 正常读取（WinAPI 验证内容一致，含中文 UTF-8）
    {
        p1 = spath("read_normal.txt");
        const char* content = "Hello SysAbs 你好喵～";
        createFileWin(wpath("read_normal.txt"), content);
        data.clear();
        auto err = fs->readAllBytes(p1, data);
        bool ok = (err == FileSystemError::Success)
            && data.size() == strlen(content)
            && memcmp(data.data(), content, data.size()) == 0;
        // 再与 WinAPI 直读结果对比
        std::vector<uint8_t> winData;
        winReadFile(wpath("read_normal.txt"), winData);
        ok = ok && data == winData;
        report(ok, "正常读取（含 UTF-8 中文）→ Success + 内容一致", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("read_normal.txt").c_str());
    }

    // 1.2 空文件
    {
        p1 = spath("read_empty.txt");
        createFileWin(wpath("read_empty.txt"), "");
        data.clear();
        auto err = fs->readAllBytes(p1, data);
        report(err == FileSystemError::Success && data.empty(), "空文件 → Success + 空内容",
            err == FileSystemError::Success ? (data.empty() ? "" : "内容不为空") : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("read_empty.txt").c_str());
    }

    // 1.3 不存在
    {
        p1 = spath("read_nonexistent.txt");
        data.clear();
        auto err = fs->readAllBytes(p1, data);
        report(err == FileSystemError::NotFound, "不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.4 目录
    {
        p1 = spath("read_dir");
        createDirWin(wpath("read_dir"));
        data.clear();
        auto err = fs->readAllBytes(p1, data);
        report(err == FileSystemError::IsDirectory, "目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("read_dir"));
    }

    // 1.5 空路径
    {
        data.clear();
        auto err = fs->readAllBytes("", data);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.6 非法字符路径
    {
        p1 = spath("illegal?.txt");
        data.clear();
        auto err = fs->readAllBytes(p1, data);
        report(err == FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.7 AccessDenied — 读取 restricted_file.txt
    if (hasRestricted) {
        p1 = zoneDir + "restricted_file.txt";
        data.clear();
        auto err = fs->readAllBytes(p1, data);
        report(err == FileSystemError::AccessDenied, "无读权限文件 → AccessDenied",
            err == FileSystemError::AccessDenied ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }
    else {
        skip("无读权限文件 → AccessDenied", "restricted_file.txt 不存在");
    }

    // 1.8 SharingViolation — 独占锁定文件
    {
        p1 = spath("read_locked.txt");
        createFileWin(wpath("read_locked.txt"), "locked");
        HANDLE hLock = CreateFileW(wpath("read_locked.txt").c_str(), GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLock == INVALID_HANDLE_VALUE) {
            skip("共享冲突 → SharingViolation", "无法以独占方式锁定文件");
        }
        else {
            data.clear();
            auto err = fs->readAllBytes(p1, data);
            report(err == FileSystemError::SharingViolation, "共享冲突 → SharingViolation",
                err == FileSystemError::SharingViolation ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
            CloseHandle(hLock);
        }
        DeleteFileW(wpath("read_locked.txt").c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 2. writeAllBytes 测试
    // =====================================================================
    std::cout << "--- writeAllBytes 测试 ---" << std::endl;

    // 2.1 正常写入（WinAPI 验证内容）
    {
        p1 = spath("write_normal.txt");
        const char* content = "write from interface 喵";
        data.assign(content, content + strlen(content));
        auto err = fs->writeAllBytes(p1, data);
        std::vector<uint8_t> winData;
        winReadFile(wpath("write_normal.txt"), winData);
        bool ok = (err == FileSystemError::Success)
            && winData.size() == strlen(content)
            && memcmp(winData.data(), content, winData.size()) == 0;
        report(ok, "正常写入 → Success（WinAPI 内容一致）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("write_normal.txt").c_str());
    }

    // 2.2 覆盖已有文件（WinAPI 验证新内容）
    {
        p1 = spath("write_overwrite.txt");
        createFileWin(wpath("write_overwrite.txt"), "old content");
        const char* newContent = "new content";
        data.assign(newContent, newContent + strlen(newContent));
        auto err = fs->writeAllBytes(p1, data);
        std::vector<uint8_t> winData;
        winReadFile(wpath("write_overwrite.txt"), winData);
        bool ok = (err == FileSystemError::Success)
            && winData.size() == strlen(newContent)
            && memcmp(winData.data(), newContent, winData.size()) == 0;
        report(ok, "覆盖已有文件 → Success（内容替换）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("write_overwrite.txt").c_str());
    }

    // 2.3 写入空数据（WinAPI 验证文件存在且大小为 0）
    {
        p1 = spath("write_empty.txt");
        data.clear();
        auto err = fs->writeAllBytes(p1, data);
        std::vector<uint8_t> winData;
        bool exists = winReadFile(wpath("write_empty.txt"), winData);
        bool ok = (err == FileSystemError::Success) && exists && winData.empty();
        report(ok, "写入空数据 → Success（空文件）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("write_empty.txt").c_str());
    }

    // 2.4 父目录不存在
    {
        p1 = spath("no_parent\\write_test.txt");
        data.assign({ 'x' });
        auto err = fs->writeAllBytes(p1, data);
        report(err == FileSystemError::NotFound, "父目录不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.5 目录
    {
        p1 = spath("write_dir");
        createDirWin(wpath("write_dir"));
        data.assign({ 'x' });
        auto err = fs->writeAllBytes(p1, data);
        report(err == FileSystemError::IsDirectory, "目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("write_dir"));
    }

    // 2.6 空路径
    {
        data.assign({ 'x' });
        auto err = fs->writeAllBytes("", data);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.7 非法字符路径
    {
        p1 = spath("illegal|.txt");
        data.assign({ 'x' });
        auto err = fs->writeAllBytes(p1, data);
        report(err == FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.8 只读文件（WinAPI 设置只读属性）
    {
        p1 = spath("write_readonly.txt");
        createFileWin(wpath("write_readonly.txt"), "readonly");
        SetFileAttributesW(wpath("write_readonly.txt").c_str(), FILE_ATTRIBUTE_READONLY);
        data.assign({ 'x' });
        auto err = fs->writeAllBytes(p1, data);
        report(err == FileSystemError::WriteProtected, "只读文件 → WriteProtected",
            err == FileSystemError::WriteProtected ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        SetFileAttributesW(wpath("write_readonly.txt").c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(wpath("write_readonly.txt").c_str());
    }

    // 2.9 AccessDenied — 写入 restricted_file.txt
    if (hasRestricted) {
        p1 = zoneDir + "restricted_file.txt";
        data.assign({ 'x' });
        auto err = fs->writeAllBytes(p1, data);
        report(err == FileSystemError::AccessDenied, "无写权限文件 → AccessDenied",
            err == FileSystemError::AccessDenied ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }
    else {
        skip("无写权限文件 → AccessDenied", "restricted_file.txt 不存在");
    }

    std::cout << std::endl;

    // =====================================================================
    // 3. appendAllBytes 测试
    // =====================================================================
    std::cout << "--- appendAllBytes 测试 ---" << std::endl;

    // 3.1 追加到已有文件（WinAPI 验证拼接结果）
    {
        p1 = spath("append_existing.txt");
        createFileWin(wpath("append_existing.txt"), "base,");
        const char* append = " add";
        data.assign(append, append + strlen(append));
        auto err = fs->appendAllBytes(p1, data);
        std::vector<uint8_t> winData;
        winReadFile(wpath("append_existing.txt"), winData);
        const char* expected = "base, add";
        bool ok = (err == FileSystemError::Success)
            && winData.size() == strlen(expected)
            && memcmp(winData.data(), expected, winData.size()) == 0;
        report(ok, "追加到已有文件 → Success（内容拼接）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("append_existing.txt").c_str());
    }

    // 3.2 追加到新文件（WinAPI 验证自动创建）
    {
        p1 = spath("append_new.txt");
        const char* content = "new from append";
        data.assign(content, content + strlen(content));
        auto err = fs->appendAllBytes(p1, data);
        std::vector<uint8_t> winData;
        bool exists = winReadFile(wpath("append_new.txt"), winData);
        bool ok = (err == FileSystemError::Success) && exists
            && winData.size() == strlen(content)
            && memcmp(winData.data(), content, winData.size()) == 0;
        report(ok, "追加到新文件 → Success（自动创建）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("append_new.txt").c_str());
    }

    // 3.3 多次追加（WinAPI 验证顺序）
    {
        p1 = spath("append_multi.txt");
        const char* parts[] = { "p1,", "p2,", "p3" };
        for (int i = 0; i < 3; ++i) {
            data.assign(parts[i], parts[i] + strlen(parts[i]));
            fs->appendAllBytes(p1, data);
        }
        std::vector<uint8_t> winData;
        winReadFile(wpath("append_multi.txt"), winData);
        const char* expected = "p1,p2,p3";
        bool ok = winData.size() == strlen(expected)
            && memcmp(winData.data(), expected, winData.size()) == 0;
        report(ok, "多次追加 → Success（顺序正确）", ok ? "" : "内容顺序错误");
        DeleteFileW(wpath("append_multi.txt").c_str());
    }

    // 3.4 追加空数据（WinAPI 验证内容不变）
    {
        p1 = spath("append_empty.txt");
        createFileWin(wpath("append_empty.txt"), "base");
        data.clear();
        auto err = fs->appendAllBytes(p1, data);
        std::vector<uint8_t> winData;
        winReadFile(wpath("append_empty.txt"), winData);
        bool ok = (err == FileSystemError::Success)
            && winData.size() == 4 && memcmp(winData.data(), "base", 4) == 0;
        report(ok, "追加空数据 → Success（内容不变）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("append_empty.txt").c_str());
    }

    // 3.5 父目录不存在
    {
        p1 = spath("no_parent\\append_test.txt");
        data.assign({ 'x' });
        auto err = fs->appendAllBytes(p1, data);
        report(err == FileSystemError::NotFound, "父目录不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.6 目录
    {
        p1 = spath("append_dir");
        createDirWin(wpath("append_dir"));
        data.assign({ 'x' });
        auto err = fs->appendAllBytes(p1, data);
        report(err == FileSystemError::IsDirectory, "目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("append_dir"));
    }

    // 3.7 空路径
    {
        data.assign({ 'x' });
        auto err = fs->appendAllBytes("", data);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.8 非法字符路径
    {
        p1 = spath("illegal?.txt");
        data.assign({ 'x' });
        auto err = fs->appendAllBytes(p1, data);
        report(err == FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.9 只读文件
    {
        p1 = spath("append_readonly.txt");
        createFileWin(wpath("append_readonly.txt"), "base");
        SetFileAttributesW(wpath("append_readonly.txt").c_str(), FILE_ATTRIBUTE_READONLY);
        data.assign({ 'x' });
        auto err = fs->appendAllBytes(p1, data);
        report(err == FileSystemError::WriteProtected, "只读文件 → WriteProtected",
            err == FileSystemError::WriteProtected ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        SetFileAttributesW(wpath("append_readonly.txt").c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(wpath("append_readonly.txt").c_str());
    }

    // 3.10 AccessDenied — 追加到 restricted_file.txt
    if (hasRestricted) {
        p1 = zoneDir + "restricted_file.txt";
        data.assign({ 'x' });
        auto err = fs->appendAllBytes(p1, data);
        report(err == FileSystemError::AccessDenied, "追加到无权限文件 → AccessDenied",
            err == FileSystemError::AccessDenied ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }
    else {
        skip("追加到无权限文件 → AccessDenied", "restricted_file.txt 不存在");
    }

    // 3.11 SharingViolation — 追加到独占锁定文件
    {
        p1 = spath("append_locked.txt");
        createFileWin(wpath("append_locked.txt"), "base");
        HANDLE hLock = CreateFileW(wpath("append_locked.txt").c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLock == INVALID_HANDLE_VALUE) {
            skip("追加到锁定文件 → SharingViolation", "无法以独占方式锁定文件");
        }
        else {
            data.assign({ 'x' });
            auto err = fs->appendAllBytes(p1, data);
            report(err == FileSystemError::SharingViolation, "追加到锁定文件 → SharingViolation",
                err == FileSystemError::SharingViolation ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
            CloseHandle(hLock);
        }
        DeleteFileW(wpath("append_locked.txt").c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 清理
    // =====================================================================
    std::cout << "--- 清理 ---" << std::endl;
    deleteDirRecursive(wTestDir);
    std::cout << "[清理] 已删除: " << zoneDir << "test_second_module" << std::endl;
    std::cout << "[清理] restricted_file.txt 未被删除（第一模块遗留）" << std::endl << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << " 测试结果汇总" << std::endl;
    std::cout << " 总计: " << totalTests << std::endl;
    std::cout << " 通过: " << passedTests << std::endl;
    std::cout << " 失败: " << failedTests << std::endl;
    std::cout << " 跳过: " << skippedTests << std::endl;
    std::cout << "========================================" << std::endl;

    return failedTests > 0 ? 1 : 0;
}
#endif