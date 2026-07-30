// ============================================================================
// test02_file_bytes_oi
// SysAbs 第二模块 “文件二进制读写” 全量测试
// 编译输出目录: $(ProjectDir)test
// 工作目录: $(ProjectDir)test\testzone
// 路径基准: 程序所在目录（exe位置）
// ============================================================================

/*
该文件（test02_file_bytes_oi）的所有代码均由 AI 生成。
*/

#include <iostream>
#include <string>
#include <vector>

#include <sysabs/ifilesystem.h>
#include <Windows.h>

// ---------------------------------------------------------------------------
// 辅助工具（WinAPI 直接操作，不依赖被测接口）
// ---------------------------------------------------------------------------

// 测试代码自用的 ChartoWide
static std::wstring test_ChartoWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
    wstr.pop_back();
    return wstr;
}

// 获取程序所在目录（不含 exe 文件名，末尾带反斜杠）
static std::string get_exe_directory() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring wpath(exePath);
    size_t pos = wpath.find_last_of(L"\\");
    if (pos != std::wstring::npos) {
        wpath = wpath.substr(0, pos + 1);  // 保留末尾反斜杠
    }
    // 转回 UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &result[0], len, nullptr, nullptr);
    result.pop_back();  // 去掉末尾空字符
    return result;
}

// 打印测试结果
static void test_pass(const char* name) {
    std::cout << "[PASS] " << name << std::endl;
}
static void test_fail(const char* name, const char* detail) {
    std::cout << "[FAIL] " << name << " : " << detail << std::endl;
}
static void test_skip(const char* name, const char* reason) {
    std::cout << "[SKIP] " << name << " : " << reason << std::endl;
}
static void test_blocking(const char* name, const char* instruction) {
    std::cout << "[BLOCKING] " << name << " : " << instruction << std::endl;
}

// 用 WinAPI 创建目录
static bool create_dir_win(const std::wstring& wpath) {
    return CreateDirectoryW(wpath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// 用 WinAPI 创建文件并写入内容
static bool create_file_win(const std::wstring& wpath, const void* data, DWORD size) {
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(h, data, size, &written, NULL) && written == size;
    CloseHandle(h);
    return ok;
}
static bool create_file_win(const std::wstring& wpath, const char* content) {
    return create_file_win(wpath, content, static_cast<DWORD>(strlen(content)));
}

// 用 WinAPI 删除文件
static bool delete_file_win(const std::wstring& wpath) {
    return DeleteFileW(wpath.c_str()) != 0;
}

// 用 WinAPI 设置文件属性
static bool set_file_attr_win(const std::wstring& wpath, DWORD attrs) {
    return SetFileAttributesW(wpath.c_str(), attrs) != 0;
}

// 用 WinAPI 递归删除目录及内容
static void delete_directory_recursive(const std::wstring& wdir) {
    std::wstring search = wdir + L"\\*";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
            continue;
        std::wstring fullPath = wdir + L"\\" + ffd.cFileName;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            delete_directory_recursive(fullPath);
        }
        else {
            SetFileAttributesW(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(fullPath.c_str());
        }
    } while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);
    RemoveDirectoryW(wdir.c_str());
}

// ---------------------------------------------------------------------------
// 测试主逻辑
// ---------------------------------------------------------------------------

int main() {
    SetConsoleOutputCP(CP_UTF8);  // ← 加上这一行

    std::cout << "========================================" << std::endl;
    std::cout << " SysAbs 第二模块全量测试 - 文件读写" << std::endl;
    std::cout << "========================================" << std::endl;

    // 以程序所在目录为基准
    std::string exeDir = get_exe_directory();
    std::cout << "[程序目录] " << exeDir << std::endl;

    // testzone 目录（程序目录下的 testzone）
    std::string zoneDir = exeDir + "testzone\\";
    std::wstring wzoneDir = test_ChartoWide(zoneDir);

    // restricted_file.txt 位于 testzone 下
    std::wstring wRestrictedFile = wzoneDir + L"restricted_file.txt";

    // 检查 restricted_file.txt 是否存在
    DWORD attr = GetFileAttributesW(wRestrictedFile.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        std::cout << "[BLOCKING] 未找到 " << zoneDir << "restricted_file.txt" << std::endl;
        std::cout << "请确保 testzone 目录存在，且其中包含 restricted_file.txt，" << std::endl;
        std::cout << "并设置其安全属性为\"仅删除权限\"，然后重新运行测试。" << std::endl;
        std::cout << "按任意键退出..." << std::endl;
        getchar();
        return 1;
    }
    std::cout << "[环境] restricted_file.txt 已找到" << std::endl;

    // 测试临时目录（在 testzone 下）
    std::wstring wTestDir = wzoneDir + L"test_second_module";
    if (!create_dir_win(wTestDir)) {
        std::cout << "[ERROR] 无法创建测试目录" << std::endl;
        return 1;
    }
    std::cout << "[环境] 测试目录已创建: " << zoneDir << "test_second_module" << std::endl;

    // 创建 IFileSystem 实例
    auto fs = sysabs::createFileSystem();
    if (!fs) {
        std::cout << "[ERROR] createFileSystem() 返回 nullptr" << std::endl;
        return 1;
    }
    std::cout << "[环境] IFileSystem 实例创建成功" << std::endl;
    std::cout << std::endl;

    // =====================================================================
    // 测试计数器
    // =====================================================================
    int totalTests = 0, passedTests = 0, failedTests = 0, skippedTests = 0;

    auto report = [&](bool ok, const char* name, const char* detail = "") {
        totalTests++;
        if (ok) { passedTests++; test_pass(name); }
        else { failedTests++; test_fail(name, detail); }
        };
    auto skip = [&](const char* name, const char* reason) {
        totalTests++; skippedTests++; test_skip(name, reason);
        };

    // 路径拼接辅助（返回 wstring，直接用于 WinAPI）
    auto wpath = [&](const char* relative) -> std::wstring {
        return wTestDir + L"\\" + test_ChartoWide(relative);
        };
    // 返回 string 版本（用于被测接口）
    auto spath = [&](const char* relative) -> std::string {
        return zoneDir + "test_second_module\\" + relative;
        };

    std::string pathBuf;
    std::vector<uint8_t> dataBuf;
    std::vector<uint8_t> readBuf;

    // =====================================================================
    // 1. readAllBytes 测试
    // =====================================================================
    std::cout << "--- readAllBytes 测试 ---" << std::endl;

    // 1.1 正常读取
    {
        pathBuf = spath("normal_read.txt");
        std::wstring wp = wpath("normal_read.txt");
        const char* content = "Hello, SysAbs! 你好喵～";
        create_file_win(wp, content);

        readBuf.clear();
        auto err = fs->readAllBytes(pathBuf, readBuf);
        bool result = (err == sysabs::FileSystemError::Success &&
            readBuf.size() == strlen(content) &&
            memcmp(readBuf.data(), content, readBuf.size()) == 0);
        report(result, "正常读取", result ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        delete_file_win(wp);
    }

    // 1.2 NotFound
    {
        pathBuf = spath("nonexistent_file.txt");
        readBuf.clear();
        auto err = fs->readAllBytes(pathBuf, readBuf);
        report(err == sysabs::FileSystemError::NotFound, "路径不存在 → NotFound",
            err == sysabs::FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.3 IsDirectory
    {
        pathBuf = zoneDir + "test_second_module";  // 目录本身
        readBuf.clear();
        auto err = fs->readAllBytes(pathBuf, readBuf);
        report(err == sysabs::FileSystemError::IsDirectory, "路径为目录 → IsDirectory",
            err == sysabs::FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.4 InvalidPath — 空路径
    {
        pathBuf = "";
        readBuf.clear();
        auto err = fs->readAllBytes(pathBuf, readBuf);
        report(err == sysabs::FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == sysabs::FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.5 InvalidPath — 非法字符
    {
        pathBuf = spath("illegal?.txt");
        readBuf.clear();
        auto err = fs->readAllBytes(pathBuf, readBuf);
        report(err == sysabs::FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == sysabs::FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.6 AccessDenied — restricted_file.txt
    {
        std::string restrictedPath = zoneDir + "restricted_file.txt";
        readBuf.clear();
        auto err = fs->readAllBytes(restrictedPath, readBuf);
        report(err == sysabs::FileSystemError::AccessDenied, "无读权限文件 → AccessDenied",
            err == sysabs::FileSystemError::AccessDenied ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 1.7 读取空文件
    {
        pathBuf = spath("empty_file.txt");
        std::wstring wp = wpath("empty_file.txt");
        create_file_win(wp, "");
        readBuf.clear();
        auto err = fs->readAllBytes(pathBuf, readBuf);
        bool result = (err == sysabs::FileSystemError::Success && readBuf.empty());
        report(result, "读取空文件 → Success + 空内容", result ? "" : ("err=" + std::to_string(static_cast<int>(err))).c_str());
        delete_file_win(wp);
    }

    // 1.8 SharingViolation
    {
        pathBuf = spath("locked_file.txt");
        std::wstring wp = wpath("locked_file.txt");
        create_file_win(wp, "locked content");

        HANDLE hLock = CreateFileW(wp.c_str(), GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLock == INVALID_HANDLE_VALUE) {
            skip("共享冲突 → SharingViolation", "无法以独占方式锁定文件");
        }
        else {
            readBuf.clear();
            auto err = fs->readAllBytes(pathBuf, readBuf);
            bool result = (err == sysabs::FileSystemError::SharingViolation);
            report(result, "共享冲突 → SharingViolation",
                result ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
            CloseHandle(hLock);
        }
        delete_file_win(wp);
    }

    std::cout << std::endl;

    // =====================================================================
    // 2. writeAllBytes 测试
    // =====================================================================
    std::cout << "--- writeAllBytes 测试 ---" << std::endl;

    // 2.1 正常写入
    {
        pathBuf = spath("normal_write.txt");
        std::wstring wp = wpath("normal_write.txt");
        const uint8_t content[] = "Hello from writeAllBytes!";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        if (err != sysabs::FileSystemError::Success) {
            report(false, "正常写入", ("write返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "正常写入", "文件创建成功但无法验证");
            }
            else {
                char verifyBuf[64] = { 0 };
                DWORD read = 0;
                ReadFile(h, verifyBuf, sizeof(verifyBuf) - 1, &read, NULL);
                CloseHandle(h);
                bool result = (read == sizeof(content) - 1 &&
                    memcmp(verifyBuf, content, read) == 0);
                report(result, "正常写入", result ? "" : "内容验证不一致");
            }
        }
        delete_file_win(wp);
    }

    // 2.2 覆盖已有文件
    {
        pathBuf = spath("overwrite_test.txt");
        std::wstring wp = wpath("overwrite_test.txt");
        create_file_win(wp, "old content");
        const uint8_t newContent[] = "new content";
        dataBuf.assign(newContent, newContent + sizeof(newContent) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        if (err != sysabs::FileSystemError::Success) {
            report(false, "覆盖已有文件", ("write返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "覆盖已有文件", "无法打开验证");
            }
            else {
                char verifyBuf[64] = { 0 };
                DWORD read = 0;
                ReadFile(h, verifyBuf, sizeof(verifyBuf) - 1, &read, NULL);
                CloseHandle(h);
                bool result = (read == sizeof(newContent) - 1 &&
                    memcmp(verifyBuf, newContent, read) == 0);
                report(result, "覆盖已有文件", result ? "" : "内容仍然是旧内容");
            }
        }
        delete_file_win(wp);
    }

    // 2.3 NotFound — 父目录不存在
    {
        pathBuf = spath("nonexistent_subdir\\write_test.txt");
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::NotFound, "父目录不存在 → NotFound",
            err == sysabs::FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.4 IsDirectory
    {
        pathBuf = zoneDir + "test_second_module";  // 目录本身
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::IsDirectory, "路径为目录 → IsDirectory",
            err == sysabs::FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.5 InvalidPath — 空路径
    {
        pathBuf = "";
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::InvalidPath, "空路径 → InvalidPath",
            err == sysabs::FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.6 InvalidPath — 非法字符
    {
        pathBuf = spath("illegal|.txt");
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::InvalidPath, "非法字符路径 → InvalidPath",
            err == sysabs::FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.7 WriteProtected — 只读文件
    {
        pathBuf = spath("readonly_write.txt");
        std::wstring wp = wpath("readonly_write.txt");
        create_file_win(wp, "read only content");
        set_file_attr_win(wp, FILE_ATTRIBUTE_READONLY);

        const uint8_t content[] = "try to write";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        bool result = (err == sysabs::FileSystemError::WriteProtected);
        report(result, "只读文件 → WriteProtected",
            result ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());

        set_file_attr_win(wp, FILE_ATTRIBUTE_NORMAL);
        delete_file_win(wp);
    }

    // 2.8 AccessDenied — restricted_file.txt
    {
        std::string restrictedPath = zoneDir + "restricted_file.txt";
        const uint8_t content[] = "try to write restricted";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->writeAllBytes(restrictedPath, dataBuf);
        report(err == sysabs::FileSystemError::AccessDenied ||
            err == sysabs::FileSystemError::WriteProtected,
            "无写权限文件 → AccessDenied",
            err == sysabs::FileSystemError::AccessDenied ? "" :
            err == sysabs::FileSystemError::WriteProtected ? "返回WriteProtected（可接受）" :
            ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 2.9 写入空内容
    {
        pathBuf = spath("write_empty.txt");
        std::wstring wp = wpath("write_empty.txt");
        dataBuf.clear();
        auto err = fs->writeAllBytes(pathBuf, dataBuf);
        if (err != sysabs::FileSystemError::Success) {
            report(false, "写入空内容", ("write返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "写入空内容", "文件未创建");
            }
            else {
                LARGE_INTEGER size;
                GetFileSizeEx(h, &size);
                CloseHandle(h);
                report(size.QuadPart == 0, "写入空内容", size.QuadPart == 0 ? "" : "文件大小不为0");
            }
        }
        delete_file_win(wp);
    }

    std::cout << std::endl;

    // =====================================================================
    // 3. appendAllBytes 测试
    // =====================================================================
    std::cout << "--- appendAllBytes 测试 ---" << std::endl;

    // 3.1 正常追加到已有文件
    {
        pathBuf = spath("append_existing.txt");
        std::wstring wp = wpath("append_existing.txt");
        create_file_win(wp, "base content,");
        const uint8_t appendContent[] = " appended content";
        dataBuf.assign(appendContent, appendContent + sizeof(appendContent) - 1);

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        if (err != sysabs::FileSystemError::Success) {
            report(false, "追加到已有文件", ("append返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "追加到已有文件", "无法打开验证");
            }
            else {
                char verifyBuf[128] = { 0 };
                DWORD read = 0;
                ReadFile(h, verifyBuf, sizeof(verifyBuf) - 1, &read, NULL);
                CloseHandle(h);
                const char* expected = "base content, appended content";
                bool result = (read == strlen(expected) && memcmp(verifyBuf, expected, read) == 0);
                report(result, "追加到已有文件", result ? "" : "内容不符合预期");
            }
        }
        delete_file_win(wp);
    }

    // 3.2 追加到新文件
    {
        pathBuf = spath("append_new.txt");
        std::wstring wp = wpath("append_new.txt");
        const uint8_t content[] = "new file from append";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        if (err != sysabs::FileSystemError::Success) {
            report(false, "追加到新文件", ("append返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "追加到新文件", "文件未创建");
            }
            else {
                char verifyBuf[64] = { 0 };
                DWORD read = 0;
                ReadFile(h, verifyBuf, sizeof(verifyBuf) - 1, &read, NULL);
                CloseHandle(h);
                bool result = (read == sizeof(content) - 1 &&
                    memcmp(verifyBuf, content, read) == 0);
                report(result, "追加到新文件", result ? "" : "内容不符合预期");
            }
        }
        delete_file_win(wp);
    }

    // 3.3 多次追加
    {
        pathBuf = spath("append_multi.txt");
        std::wstring wp = wpath("append_multi.txt");
        const uint8_t part1[] = "part1,";
        dataBuf.assign(part1, part1 + sizeof(part1) - 1);
        fs->appendAllBytes(pathBuf, dataBuf);
        const uint8_t part2[] = "part2,";
        dataBuf.assign(part2, part2 + sizeof(part2) - 1);
        fs->appendAllBytes(pathBuf, dataBuf);
        const uint8_t part3[] = "part3";
        dataBuf.assign(part3, part3 + sizeof(part3) - 1);
        auto err = fs->appendAllBytes(pathBuf, dataBuf);

        if (err != sysabs::FileSystemError::Success) {
            report(false, "多次追加", ("第三次append返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "多次追加", "无法打开验证");
            }
            else {
                char verifyBuf[64] = { 0 };
                DWORD read = 0;
                ReadFile(h, verifyBuf, sizeof(verifyBuf) - 1, &read, NULL);
                CloseHandle(h);
                const char* expected = "part1,part2,part3";
                bool result = (read == strlen(expected) && memcmp(verifyBuf, expected, read) == 0);
                report(result, "多次追加", result ? "" : "内容不符合预期");
            }
        }
        delete_file_win(wp);
    }

    // 3.4 NotFound — 父目录不存在
    {
        pathBuf = spath("nonexistent_subdir\\append_test.txt");
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::NotFound, "父目录不存在追加 → NotFound",
            err == sysabs::FileSystemError::NotFound ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.5 IsDirectory
    {
        pathBuf = zoneDir + "test_second_module";  // 目录本身
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::IsDirectory, "目录追加 → IsDirectory",
            err == sysabs::FileSystemError::IsDirectory ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.6 WriteProtected — 追加到只读文件
    {
        pathBuf = spath("readonly_append.txt");
        std::wstring wp = wpath("readonly_append.txt");
        create_file_win(wp, "read only");
        set_file_attr_win(wp, FILE_ATTRIBUTE_READONLY);

        const uint8_t content[] = " try to append";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        bool result = (err == sysabs::FileSystemError::WriteProtected);
        report(result, "追加到只读文件 → WriteProtected",
            result ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());

        set_file_attr_win(wp, FILE_ATTRIBUTE_NORMAL);
        delete_file_win(wp);
    }

    // 3.7 AccessDenied — restricted_file.txt
    {
        std::string restrictedPath = zoneDir + "restricted_file.txt";
        const uint8_t content[] = "try to append to restricted";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->appendAllBytes(restrictedPath, dataBuf);
        report(err == sysabs::FileSystemError::AccessDenied ||
            err == sysabs::FileSystemError::WriteProtected,
            "追加到无权限文件 → AccessDenied",
            err == sysabs::FileSystemError::AccessDenied ? "" :
            err == sysabs::FileSystemError::WriteProtected ? "返回WriteProtected（可接受）" :
            ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.8 InvalidPath — 空路径
    {
        pathBuf = "";
        const uint8_t content[] = "test";
        dataBuf.assign(content, content + sizeof(content) - 1);

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        report(err == sysabs::FileSystemError::InvalidPath, "空路径追加 → InvalidPath",
            err == sysabs::FileSystemError::InvalidPath ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
    }

    // 3.9 追加空内容
    {
        pathBuf = spath("append_empty.txt");
        std::wstring wp = wpath("append_empty.txt");
        create_file_win(wp, "base");
        dataBuf.clear();

        auto err = fs->appendAllBytes(pathBuf, dataBuf);
        if (err != sysabs::FileSystemError::Success) {
            report(false, "追加空内容", ("append返回: " + std::to_string(static_cast<int>(err))).c_str());
        }
        else {
            HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                report(false, "追加空内容", "无法打开验证");
            }
            else {
                char verifyBuf[16] = { 0 };
                DWORD read = 0;
                ReadFile(h, verifyBuf, sizeof(verifyBuf) - 1, &read, NULL);
                CloseHandle(h);
                bool result = (read == 4 && memcmp(verifyBuf, "base", 4) == 0);
                report(result, "追加空内容", result ? "" : "内容被修改");
            }
        }
        delete_file_win(wp);
    }

    // 3.10 SharingViolation
    {
        pathBuf = spath("locked_append.txt");
        std::wstring wp = wpath("locked_append.txt");
        create_file_win(wp, "locked");

        HANDLE hLock = CreateFileW(wp.c_str(), GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLock == INVALID_HANDLE_VALUE) {
            skip("追加到锁定文件 → SharingViolation", "无法以独占方式锁定文件");
        }
        else {
            const uint8_t content[] = " try to append";
            dataBuf.assign(content, content + sizeof(content) - 1);
            auto err = fs->appendAllBytes(pathBuf, dataBuf);
            bool result = (err == sysabs::FileSystemError::SharingViolation);
            report(result, "追加到锁定文件 → SharingViolation",
                result ? "" : ("实际: " + std::to_string(static_cast<int>(err))).c_str());
            CloseHandle(hLock);
        }
        delete_file_win(wp);
    }

    std::cout << std::endl;

    // =====================================================================
    // 清理
    // =====================================================================
    std::cout << "--- 清理 ---" << std::endl;
    delete_directory_recursive(wTestDir);
    std::cout << "[清理] 已删除: " << zoneDir << "test_second_module" << std::endl;
    std::cout << "[清理] restricted_file.txt 未被删除（第一模块遗留）" << std::endl;
    std::cout << std::endl;

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