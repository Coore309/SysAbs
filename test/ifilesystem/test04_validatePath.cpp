#if 0
// ============================================================================
// test_validate_path.cpp
// SysAbs validatePath 全量测试 - 路径合法性校验
// 编译输出目录: $(ProjectDir)test
// 操作走接口（validatePath 为纯字符串校验，不涉及文件系统操作）
// ============================================================================

/*
该文件的所有代码均由 AI 生成。
*/

#include <iostream>
#include <string>
#include <Windows.h>

#include <sysabs/ifilesystem.h>

using namespace sysabs;

static void testPass(const char* name) { std::cout << "[PASS] " << name << std::endl; }
static void testFail(const char* name, const char* detail) { std::cout << "[FAIL] " << name << " : " << detail << std::endl; }

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "========================================" << std::endl;
    std::cout << " validatePath 全量测试 - 路径合法性校验" << std::endl;
    std::cout << "========================================" << std::endl;

    auto fs = createFileSystem();
    if (!fs) { std::cout << "[ERROR] createFileSystem() 返回 nullptr" << std::endl; return 1; }
    std::cout << "[环境] IFileSystem 实例创建成功" << std::endl << std::endl;

    int totalTests = 0, passedTests = 0, failedTests = 0;
    auto report = [&](bool ok, const char* name, const char* detail = "") {
        totalTests++;
        if (ok) { passedTests++; testPass(name); }
        else { failedTests++; testFail(name, detail); }
        };

    // =====================================================================
    // 1. 非法输入测试
    // =====================================================================
    std::cout << "--- 非法输入测试 ---" << std::endl;

    // 1.1 空路径
    report(!fs->validatePath(""), "空路径 → false");

    // 1.2 超长路径（>= MAX_PATH = 260）
    report(!fs->validatePath(std::string(260, 'a')), "长度 260（MAX_PATH）→ false");
    report(fs->validatePath(std::string(259, 'a')), "长度 259（MAX_PATH-1）→ true");

    // 1.3 控制字符（ASCII 0-31）
    report(!fs->validatePath("D:/fish/na\x01me.txt"), "控制字符 0x01 → false");
    report(!fs->validatePath("D:/fish/na\tme.txt"), "控制字符 TAB → false");
    report(!fs->validatePath("D:/fish/na\nme.txt"), "控制字符 LF → false");
    report(!fs->validatePath("D:/fish/na\rme.txt"), "控制字符 CR → false");

    // 1.4 DEL 字符（ASCII 127）
    report(!fs->validatePath("D:/fish/na\x7fme.txt"), "DEL 字符 (127) → false");

    // 1.5 Windows 非法字符
    report(!fs->validatePath("D:/fish/na?me.txt"), "非法字符 ? → false");
    report(!fs->validatePath("D:/fish/na*me.txt"), "非法字符 * → false");
    report(!fs->validatePath("D:/fish/na<me.txt"), "非法字符 < → false");
    report(!fs->validatePath("D:/fish/na>me.txt"), "非法字符 > → false");
    report(!fs->validatePath("D:/fish/na\"me.txt"), "非法字符 \" → false");
    report(!fs->validatePath("D:/fish/na|me.txt"), "非法字符 | → false");

    // 1.6 非法盘符（第一个字符不是字母）
    report(!fs->validatePath("1:/fish"), "数字盘符 1: → false");
    report(!fs->validatePath("-:/fish"), "符号盘符 -: → false");
    report(!fs->validatePath(":/fish"), "空盘符 : → false");

    // 1.7 冒号出现在非盘符位置
    report(!fs->validatePath("C:a:b"), "多余冒号（C:a:b）→ false");
    report(!fs->validatePath("D:/fish/a:b"), "路径中段冒号 → false");

    std::cout << std::endl;

    // =====================================================================
    // 2. 合法输入测试
    // =====================================================================
    std::cout << "--- 合法输入测试 ---" << std::endl;

    // 2.1 标准绝对路径
    report(fs->validatePath("C:/fish/nemo.txt"), "正斜杠绝对路径 → true");
    report(fs->validatePath("D:\\fish\\nemo.txt"), "反斜杠绝对路径 → true");

    // 2.2 大小写盘符
    report(fs->validatePath("C:/fish"), "大写盘符 → true");
    report(fs->validatePath("c:/fish"), "小写盘符 → true");

    // 2.3 仅盘符
    report(fs->validatePath("C:"), "仅盘符 C: → true");

    // 2.4 盘符相对路径（drive-relative）
    report(fs->validatePath("C:fish"), "盘符相对路径 C:fish → true");

    // 2.5 相对路径
    report(fs->validatePath("fish/nemo.txt"), "相对路径（正斜杠）→ true");
    report(fs->validatePath("fish\\nemo.txt"), "相对路径（反斜杠）→ true");

    // 2.6 无盘符绝对路径
    report(fs->validatePath("/fish/nemo.txt"), "无盘符绝对路径 → true");

    // 2.7 中文路径（UTF-8 多字节字符不应被误判为控制字符）
    report(fs->validatePath("D:/鱼/尼莫.txt"), "中文路径 → true");
    report(fs->validatePath("D:/测试目录/文件.bin"), "中文目录 + 中文文件名 → true");

    // 2.8 含点号路径（. 和 .. 是合法的路径段）
    report(fs->validatePath("D:/fish/./nemo.txt"), "含单点路径段 → true");
    report(fs->validatePath("D:/fish/../nemo.txt"), "含双点路径段 → true");

    // 2.9 混合分隔符
    report(fs->validatePath("D:/fish\\sub/nemo.txt"), "混合分隔符 → true");

    // 2.10 多级深层路径
    report(fs->validatePath("C:/a/b/c/d/e/f/g/h.txt"), "深层目录路径 → true");

    // 2.11 文件名含空格
    report(fs->validatePath("D:/fish/my fish.txt"), "文件名含空格 → true");

    std::cout << std::endl;

    // =====================================================================
    // 汇总
    // =====================================================================
    std::cout << "========================================" << std::endl;
    std::cout << " 测试结果汇总" << std::endl;
    std::cout << " 总计: " << totalTests << std::endl;
    std::cout << " 通过: " << passedTests << std::endl;
    std::cout << " 失败: " << failedTests << std::endl;
    std::cout << "========================================" << std::endl;

    return failedTests > 0 ? 1 : 0;
}
#endif