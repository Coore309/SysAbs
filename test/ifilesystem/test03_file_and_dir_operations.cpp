#if 0
// ============================================================================
// test03_file_and_dir_operations.cpp
// SysAbs 第三模块全量测试 - 文件与目录操作
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

static const char* TEST_DIR_NAME = "test_third_module";
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

// ── 环境准备 ──

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

// ── 验证辅助（全部 WinAPI，独立于被测接口） ──

// 检查路径是否存在
static bool winPathExists(const std::wstring& wpath) {
    return GetFileAttributesW(wpath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// 检查路径是否还是目录
static bool winIsDirectory(const std::wstring& wpath) {
    DWORD attr = GetFileAttributesW(wpath.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

// 读取文件内容（WinAPI），返回字节串
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

// 清理：递归删除目录
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
    std::cout << " SysAbs 第三模块全量测试 - 文件与目录操作" << std::endl;
    std::cout << "========================================" << std::endl;

    // 路径基准
    std::string exeDir = getExeDirectory();
    std::string zoneDir = exeDir + "testzone\\";
    std::wstring wZoneDir = testChartoWide(zoneDir);
    std::wstring wRestricted = wZoneDir + L"restricted_file.txt";
    std::wstring wTestDir = wZoneDir + L"test_third_module";

    // 环境检查
    bool hasRestricted = winPathExists(wRestricted);
    std::cout << "[环境] restricted_file.txt: " << (hasRestricted ? "已找到" : "未找到") << std::endl;

    if (!createDirWin(wTestDir)) {
        std::cout << "[ERROR] 无法创建测试目录" << std::endl;
        return 1;
    }
    std::cout << "[环境] 测试目录: " << zoneDir << "test_third_module" << std::endl;

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

    // 路径拼接辅助
    auto wpath = [&](const char* rel) -> std::wstring { return wTestDir + L"\\" + testChartoWide(rel); };
    auto spath = [&](const char* rel) -> std::string { return zoneDir + "test_third_module\\" + rel; };

    std::string p1, p2;
    std::vector<uint8_t> buf;

    // =====================================================================
    // 1. validatePath 测试
    // =====================================================================
    std::cout << "--- validatePath 测试 ---" << std::endl;

    report(!fs->validatePath(""), "空路径 → false");
    report(fs->validatePath("C:/fish/nemo.txt"), "合法路径（正斜杠）→ true");
    report(fs->validatePath("D:\\fish\\nemo.txt"), "合法路径（反斜杠）→ true");
    report(!fs->validatePath("D:/fish/na?me.txt"), "非法字符 ? → false");
    report(!fs->validatePath("D:/fish/na*me.txt"), "非法字符 * → false");
    report(!fs->validatePath("D:/fish/na<me.txt"), "非法字符 < → false");
    report(!fs->validatePath("D:/fish/na|me.txt"), "非法字符 | → false");
    report(!fs->validatePath("1:/fish"), "非法盘符（数字）→ false");
    report(fs->validatePath("C:/fish"), "大写盘符 → true");
    report(fs->validatePath("c:/fish"), "小写盘符 → true");
    report(!fs->validatePath("D:/fish/na\tme.txt"), "控制字符 → false");

    std::cout << std::endl;

    // =====================================================================
    // 2. listDirectory 测试
    // =====================================================================
    std::cout << "--- listDirectory 测试 ---" << std::endl;

    // 2.1 正常列出（WinAPI 验证条目存在）
    {
        p1 = spath("list_normal");
        createDirWin(wpath("list_normal"));
        createFileWin(wpath("list_normal\\a.txt"), "A");
        createDirWin(wpath("list_normal\\subdir"));

        std::vector<DirectoryEntry> entries;
        auto err = fs->listDirectory(p1, entries);
        bool result = (err == FileSystemError::Success && entries.size() == 2);
        // 用 WinAPI 确认这两个条目真实存在
        if (result) {
            result = winPathExists(wpath("list_normal\\a.txt"))
                && winIsDirectory(wpath("list_normal\\subdir"));
        }
        report(result, "正常列出（文件+目录）", result ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());

        deleteDirRecursive(wpath("list_normal"));
    }

    // 2.2 空目录
    {
        p1 = spath("list_empty");
        createDirWin(wpath("list_empty"));

        std::vector<DirectoryEntry> entries;
        auto err = fs->listDirectory(p1, entries);
        report(err == FileSystemError::Success && entries.empty(), "空目录 → Success + 0条",
            err == FileSystemError::Success ? (entries.empty() ? "" : "条目数不为0") : ("err=" + std::to_string(static_cast<int>(err))).c_str());

        deleteDirRecursive(wpath("list_empty"));
    }

    // 2.3 目录不存在
    {
        p1 = spath("list_nonexistent");
        std::vector<DirectoryEntry> entries;
        auto err = fs->listDirectory(p1, entries);
        report(err == FileSystemError::NotFound, "目录不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.4 路径为文件
    {
        p1 = spath("list_is_file.txt");
        createFileWin(wpath("list_is_file.txt"), "data");
        std::vector<DirectoryEntry> entries;
        auto err = fs->listDirectory(p1, entries);
        report(err == FileSystemError::IsFile, "路径为文件 → IsFile",
            err == FileSystemError::IsFile ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("list_is_file.txt").c_str());
    }

    // 2.5 空路径
    {
        std::vector<DirectoryEntry> entries;
        auto err = fs->listDirectory("", entries);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.6 非法字符路径
    {
        p1 = spath("illegal?.txt");
        std::vector<DirectoryEntry> entries;
        auto err = fs->listDirectory(p1, entries);
        report(err == FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 3. moveFile 测试
    // =====================================================================
    std::cout << "--- moveFile 测试 ---" << std::endl;

    // 3.1 正常移动（WinAPI 验证源消失、目标存在）
    {
        p1 = spath("move_src.txt");
        p2 = spath("move_dst.txt");
        createFileWin(wpath("move_src.txt"), "hello");
        auto err = fs->moveFile(p1, p2, false);
        bool ok = (err == FileSystemError::Success)
            && !winPathExists(wpath("move_src.txt"))
            && winPathExists(wpath("move_dst.txt"));
        report(ok, "正常移动 → Success（源消失目标存在）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("move_dst.txt").c_str());
    }

    // 3.2 重命名
    {
        p1 = spath("rename_old.txt");
        p2 = spath("rename_new.txt");
        createFileWin(wpath("rename_old.txt"), "x");
        auto err = fs->moveFile(p1, p2, false);
        bool ok = (err == FileSystemError::Success)
            && !winPathExists(wpath("rename_old.txt"))
            && winPathExists(wpath("rename_new.txt"));
        report(ok, "重命名 → Success", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("rename_new.txt").c_str());
    }

    // 3.3 源不存在
    {
        p1 = spath("move_nonexistent.txt");
        p2 = spath("move_nodst.txt");
        auto err = fs->moveFile(p1, p2, false);
        report(err == FileSystemError::NotFound, "源不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.4 源是目录
    {
        p1 = spath("move_is_dir");
        p2 = spath("move_dir_to.txt");
        createDirWin(wpath("move_is_dir"));
        auto err = fs->moveFile(p1, p2, false);
        report(err == FileSystemError::IsDirectory, "源是目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("move_is_dir"));
    }

    // 3.5 目标已存在且不覆盖
    {
        p1 = spath("move_cover_src.txt");
        p2 = spath("move_cover_dst.txt");
        createFileWin(wpath("move_cover_src.txt"), "src");
        createFileWin(wpath("move_cover_dst.txt"), "dst");
        auto err = fs->moveFile(p1, p2, false);
        // 不覆盖时：源和目标都应还在
        bool ok = (err == FileSystemError::AlreadyExists)
            && winPathExists(wpath("move_cover_src.txt"))
            && winPathExists(wpath("move_cover_dst.txt"));
        report(ok, "目标已存在不覆盖 → AlreadyExists（双方保留）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("move_cover_src.txt").c_str());
        DeleteFileW(wpath("move_cover_dst.txt").c_str());
    }

    // 3.6 目标已存在且覆盖（WinAPI 验证内容被替换）
    {
        p1 = spath("move_cover2_src.txt");
        p2 = spath("move_cover2_dst.txt");
        createFileWin(wpath("move_cover2_src.txt"), "new");
        createFileWin(wpath("move_cover2_dst.txt"), "old");
        auto err = fs->moveFile(p1, p2, true);
        std::vector<uint8_t> content;
        bool contentOk = winReadFile(wpath("move_cover2_dst.txt"), content);
        bool ok = (err == FileSystemError::Success)
            && !winPathExists(wpath("move_cover2_src.txt"))
            && contentOk
            && content.size() == 3 && memcmp(content.data(), "new", 3) == 0;
        report(ok, "目标已存在且覆盖 → Success（内容替换）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("move_cover2_dst.txt").c_str());
    }

    // 3.7 空路径
    {
        auto err = fs->moveFile("", "x.txt", false);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 4. copyFile 测试
    // =====================================================================
    std::cout << "--- copyFile 测试 ---" << std::endl;

    // 4.1 正常复制（WinAPI 验证内容一致）
    {
        p1 = spath("copy_src.txt");
        p2 = spath("copy_dst.txt");
        const char* content = "copy content 喵";
        createFileWin(wpath("copy_src.txt"), content);
        auto err = fs->copyFile(p1, p2, false);
        if (err != FileSystemError::Success) {
            report(false, "正常复制 → Success", ("err=" + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            std::vector<uint8_t> dstContent;
            bool ok = winReadFile(wpath("copy_dst.txt"), dstContent)
                && dstContent.size() == strlen(content)
                && memcmp(dstContent.data(), content, dstContent.size()) == 0;
            report(ok, "正常复制 → Success（WinAPI 内容一致）", ok ? "" : "内容不一致");
        }
        DeleteFileW(wpath("copy_src.txt").c_str());
        DeleteFileW(wpath("copy_dst.txt").c_str());
    }

    // 4.2 源不存在
    {
        p1 = spath("copy_nonexistent.txt");
        p2 = spath("copy_nodst.txt");
        auto err = fs->copyFile(p1, p2, false);
        report(err == FileSystemError::NotFound, "源不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 4.3 源是目录
    {
        p1 = spath("copy_is_dir");
        p2 = spath("copy_dst.txt");
        createDirWin(wpath("copy_is_dir"));
        auto err = fs->copyFile(p1, p2, false);
        report(err == FileSystemError::IsDirectory, "源是目录 → IsDirectory",
            err == FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("copy_is_dir"));
    }

    // 4.4 目标已存在且不覆盖（WinAPI 验证目标内容未被改）
    {
        p1 = spath("copy_cover_src.txt");
        p2 = spath("copy_cover_dst.txt");
        createFileWin(wpath("copy_cover_src.txt"), "src");
        createFileWin(wpath("copy_cover_dst.txt"), "dst");
        auto err = fs->copyFile(p1, p2, false);
        std::vector<uint8_t> dstContent;
        winReadFile(wpath("copy_cover_dst.txt"), dstContent);
        bool ok = (err == FileSystemError::AlreadyExists)
            && dstContent.size() == 3 && memcmp(dstContent.data(), "dst", 3) == 0;
        report(ok, "目标已存在不覆盖 → AlreadyExists（目标内容未变）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("copy_cover_src.txt").c_str());
        DeleteFileW(wpath("copy_cover_dst.txt").c_str());
    }

    // 4.5 覆盖复制（WinAPI 验证内容被替换）
    {
        p1 = spath("copy_cover2_src.txt");
        p2 = spath("copy_cover2_dst.txt");
        createFileWin(wpath("copy_cover2_src.txt"), "new");
        createFileWin(wpath("copy_cover2_dst.txt"), "old");
        auto err = fs->copyFile(p1, p2, true);
        std::vector<uint8_t> dstContent;
        winReadFile(wpath("copy_cover2_dst.txt"), dstContent);
        bool ok = (err == FileSystemError::Success)
            && dstContent.size() == 3 && memcmp(dstContent.data(), "new", 3) == 0;
        report(ok, "覆盖复制 → Success（内容替换）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("copy_cover2_src.txt").c_str());
        DeleteFileW(wpath("copy_cover2_dst.txt").c_str());
    }

    // 4.6 目标父目录不存在
    {
        p1 = spath("copy_nodir_src.txt");
        p2 = spath("no_subdir\\copy_dst.txt");
        createFileWin(wpath("copy_nodir_src.txt"), "x");
        auto err = fs->copyFile(p1, p2, false);
        report(err == FileSystemError::NotFound, "目标父目录不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("copy_nodir_src.txt").c_str());
    }

    // 4.7 AccessDenied — 从 restricted_file.txt 复制
    if (hasRestricted) {
        p1 = zoneDir + "restricted_file.txt";
        p2 = spath("copy_restricted_dst.txt");
        auto err = fs->copyFile(p1, p2, false);
        bool ok = (err == FileSystemError::AccessDenied) && !winPathExists(wpath("copy_restricted_dst.txt"));
        report(ok, "读取受限源 → AccessDenied（目标未创建）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("copy_restricted_dst.txt").c_str());
    }
    else {
        skip("读取受限源 → AccessDenied", "restricted_file.txt 不存在");
    }

    // 4.8 空路径
    {
        auto err = fs->copyFile("", "x.txt", false);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 5. deleteFile 测试
    // =====================================================================
    std::cout << "--- deleteFile 测试 ---" << std::endl;

    // 5.1 正常删除（WinAPI 验证文件消失）
    {
        p1 = spath("delete_ok.txt");
        createFileWin(wpath("delete_ok.txt"), "x");
        auto err = fs->deleteFile(p1);
        bool ok = (err == FileSystemError::Success) && !winPathExists(wpath("delete_ok.txt"));
        report(ok, "正常删除 → Success（文件消失）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
    }

    // 5.2 文件不存在
    {
        p1 = spath("delete_nonexistent.txt");
        auto err = fs->deleteFile(p1);
        report(err == FileSystemError::NotFound, "文件不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 5.3 路径是目录
    {
        p1 = spath("delete_is_dir");
        createDirWin(wpath("delete_is_dir"));
        auto err = fs->deleteFile(p1);
        bool ok = (err == FileSystemError::IsDirectory) && winIsDirectory(wpath("delete_is_dir"));
        report(ok, "路径是目录 → IsDirectory（目录仍在）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("delete_is_dir"));
    }

    // 5.4 空路径
    {
        auto err = fs->deleteFile("");
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 6. createDirectory 测试
    // =====================================================================
    std::cout << "--- createDirectory 测试 ---" << std::endl;

    // 6.1 正常创建（WinAPI 验证目录存在）
    {
        p1 = spath("create_ok");
        auto err = fs->createDirectory(p1);
        bool ok = (err == FileSystemError::Success) && winIsDirectory(wpath("create_ok"));
        report(ok, "正常创建 → Success（目录存在）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("create_ok"));
    }

    // 6.2 目录已存在
    {
        p1 = spath("create_exists");
        createDirWin(wpath("create_exists"));
        auto err = fs->createDirectory(p1);
        report(err == FileSystemError::AlreadyExists, "目录已存在 → AlreadyExists",
            err == FileSystemError::AlreadyExists ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("create_exists"));
    }

    // 6.3 同名文件已存在
    {
        p1 = spath("create_is_file.txt");
        createFileWin(wpath("create_is_file.txt"), "x");
        auto err = fs->createDirectory(p1);
        bool ok = (err == FileSystemError::IsFile) && !winIsDirectory(wpath("create_is_file.txt"));
        report(ok, "同名文件已存在 → IsFile（未创建目录）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("create_is_file.txt").c_str());
    }

    // 6.4 父目录不存在
    {
        p1 = spath("no_parent\\create_sub");
        auto err = fs->createDirectory(p1);
        report(err == FileSystemError::NotFound, "父目录不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 6.5 空路径
    {
        auto err = fs->createDirectory("");
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 7. moveDirectory 测试
    // =====================================================================
    std::cout << "--- moveDirectory 测试 ---" << std::endl;

    // 7.1 正常移动（WinAPI 验证源消失、目标内容存在）
    {
        p1 = spath("mvdir_src");
        p2 = spath("mvdir_dst");
        createDirWin(wpath("mvdir_src"));
        createFileWin(wpath("mvdir_src\\a.txt"), "x");
        auto err = fs->moveDirectory(p1, p2, false);
        bool ok = (err == FileSystemError::Success)
            && !winPathExists(wpath("mvdir_src"))
            && winPathExists(wpath("mvdir_dst\\a.txt"));
        report(ok, "正常移动（含内容）→ Success", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("mvdir_dst"));
    }

    // 7.2 源不存在
    {
        p1 = spath("mvdir_nonexistent");
        p2 = spath("mvdir_nodst");
        auto err = fs->moveDirectory(p1, p2, false);
        report(err == FileSystemError::NotFound, "源不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 7.3 源是文件
    {
        p1 = spath("mvdir_is_file.txt");
        p2 = spath("mvdir_from_file");
        createFileWin(wpath("mvdir_is_file.txt"), "x");
        auto err = fs->moveDirectory(p1, p2, false);
        bool ok = (err == FileSystemError::IsFile) && winPathExists(wpath("mvdir_is_file.txt"));
        report(ok, "源是文件 → IsFile（文件未移动）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("mvdir_is_file.txt").c_str());
    }

    // 7.4 目标已存在且不覆盖
    {
        p1 = spath("mvdir_cover_src");
        p2 = spath("mvdir_cover_dst");
        createDirWin(wpath("mvdir_cover_src"));
        createDirWin(wpath("mvdir_cover_dst"));
        auto err = fs->moveDirectory(p1, p2, false);
        bool ok = (err == FileSystemError::AlreadyExists)
            && winIsDirectory(wpath("mvdir_cover_src"))
            && winIsDirectory(wpath("mvdir_cover_dst"));
        report(ok, "目标已存在不覆盖 → AlreadyExists（双方保留）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("mvdir_cover_src"));
        deleteDirRecursive(wpath("mvdir_cover_dst"));
    }

    // 7.5 目标已存在且是文件
    {
        p1 = spath("mvdir_target_file_src");
        p2 = spath("mvdir_target_file.txt");
        createDirWin(wpath("mvdir_target_file_src"));
        createFileWin(wpath("mvdir_target_file.txt"), "x");
        auto err = fs->moveDirectory(p1, p2, false);
        bool ok = (err == FileSystemError::IsFile)
            && winIsDirectory(wpath("mvdir_target_file_src"))
            && winPathExists(wpath("mvdir_target_file.txt"));
        report(ok, "目标已存在且是文件 → IsFile（双方保留）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("mvdir_target_file_src"));
        DeleteFileW(wpath("mvdir_target_file.txt").c_str());
    }

    // 7.6 空路径
    {
        auto err = fs->moveDirectory("", "x", false);
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 8. removeDirectory 测试
    // =====================================================================
    std::cout << "--- removeDirectory 测试 ---" << std::endl;

    // 8.1 正常删除空目录（WinAPI 验证目录消失）
    {
        p1 = spath("rmdir_ok");
        createDirWin(wpath("rmdir_ok"));
        auto err = fs->removeDirectory(p1);
        bool ok = (err == FileSystemError::Success) && !winPathExists(wpath("rmdir_ok"));
        report(ok, "删除空目录 → Success（目录消失）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
    }

    // 8.2 目录不存在
    {
        p1 = spath("rmdir_nonexistent");
        auto err = fs->removeDirectory(p1);
        report(err == FileSystemError::NotFound, "目录不存在 → NotFound",
            err == FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 8.3 目录非空（WinAPI 验证目录仍在）
    {
        p1 = spath("rmdir_not_empty");
        createDirWin(wpath("rmdir_not_empty"));
        createFileWin(wpath("rmdir_not_empty\\a.txt"), "x");
        auto err = fs->removeDirectory(p1);
        bool ok = (err == FileSystemError::DirectoryNotEmpty) && winIsDirectory(wpath("rmdir_not_empty"));
        report(ok, "目录非空 → DirectoryNotEmpty（目录仍在）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        deleteDirRecursive(wpath("rmdir_not_empty"));
    }

    // 8.4 路径是文件
    {
        p1 = spath("rmdir_is_file.txt");
        createFileWin(wpath("rmdir_is_file.txt"), "x");
        auto err = fs->removeDirectory(p1);
        bool ok = (err == FileSystemError::IsFile) && winPathExists(wpath("rmdir_is_file.txt"));
        report(ok, "路径是文件 → IsFile（文件仍在）", ok ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        DeleteFileW(wpath("rmdir_is_file.txt").c_str());
    }

    // 8.5 空路径
    {
        auto err = fs->removeDirectory("");
        report(err == FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    std::cout << std::endl;

    // =====================================================================
    // 清理
    // =====================================================================
    std::cout << "--- 清理 ---" << std::endl;
    deleteDirRecursive(wTestDir);
    std::cout << "[清理] 已删除: " << zoneDir << "test_third_module" << std::endl;
    std::cout << "[清理] restricted_file.txt 未被删除（第一模块遗留）" << std::endl << std::endl;

    // =====================================================================
    // 汇总
    // =====================================================================
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