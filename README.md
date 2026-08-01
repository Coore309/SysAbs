# SysAbs
一个简单的跨平台系统抽象库，为上层应用提供统一的系统底层API访问接口。（自用）
*Windows 平台文件系统模块* 已开发完成。

## 特性
- **工厂模式** 通过 `createFileSystem()` 等统一的实例化函数获取对应平台实例
- **统一错误处理** 所有接口操作返回错误码枚举，详细错误信息由 `getLastErrorMessage()` 返回
- **路径风格兼容** `/` 与 `\` 均可正常解析

## 安装与构建
### 手动链接
1. 将 `include/` 加入 C/C++ &rarr; 常规 &rarr; 附加包含目录
2. 将 `SystemAbstraction.lib` 加入链接器 &rarr; 输入 &rarr; 附加依赖项
```cpp
#pragma comment(lib, "SysAbs.lib")
```

### 从源码构建
``` bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## API概览
### IFileSystem

|作用类别|接口|
|:------:|:----|
|元数据查询|exists getFileType getFileSize getLastModifiedTime|
|文件读写|readAllBytes writeAllBytes appendAllBytes|
|文件操作|moveFile copyFile deleteFile|
目录操作|createDirectory listDirectory moveDirectory removeDirectory|

错误返回 `FileSystemError`

## 注意事项
- 库与调用方需使用相同的运行库设置
- 接口统一接受 UTF-8 字符
- `removeDirectory` 仅删除空目录，递归删除请自行封装

## 许可证
本项目基于 **MIT 许可证** 开源发布，详细许可证文本见 [LICENSE](LICENSE) 文件