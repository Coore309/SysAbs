#pragma once

#include <string>

/**
 * \brief 文件系统操作错误码枚举
 * 
 * 所有 IFileSystem 方法均返回该枚举，表示简单表示操作成功或失败抑或是失败的大致原因。详细错误信息请使用 GetLastErrorf() 方法获取。
 */
enum class FileSystemError {
	// 成功通用符
	Success = 0,
	// 失败通用符
	Failure,
	// 程序拥有的权限不足
	AccessDenied,
	// 路径无效，存在非法字符、无法正常解析路径或路径为空
	InvalidPath,
	// 路径不存在
	NotFound,
	// 路径为目录（期望为文件时）
	IsDirectory,
	// 未知错误
	UnKnown
};

/**
 * \brief 枚举路径指向的类型
 * 
 * 只用于 getFileType 函数。
 */
enum class FileType { 
	// 文件
	File,
	// 目录
	Directory,
	// 未找到路径
	NotFound,
	// 未知类型
	Unknown
};

/**
 * \brief 文件系统抽象接口
 * 
 * 所有方法均返回 FileSystemError，通过返回值即可判断大致结果、错误内容。
 * 所有成功与正向结果均用 Success 表示。
 * 所有的复杂数据全部使用引用参数传递。
 */
class IFileSystem {
public:
	virtual ~IFileSystem() = default;

	// 获取最后一次错误消息
	std::string getLastErrorMessage() const {
		return error_message_;
	}

	/*
	* 文件存在性及其元数据查询
	*///

	/**
	 * \brief 检查文件或目录是否存在
	 * 
	 * \param path 路径
	 * \return FileSystemError::Success 文件存在
	 *         FileSystemError::NotFound 文件不存在
	 *         FileSystemError::Invaild 无效路径
	 *         FileSystemError::AccessDenied 程序权限不足
	 *         FileSystemError::UnKnown 未知错误
	 */
	virtual FileSystemError exists(const std::string& path) = 0;

	/**
	 * \brief 获取路径对于的类型
	 * 
	 * \param path 路径
	 * \param[out] outType 输出参数类型为 FileType ，返回以下结果：
	 *						- FileType::File: 路径为文件
	 *						- FileType::Directory: 路径为目录
	 *						- FileType::NotFound: 路径不存在
	 *						- FileType::Unknown: 路径存在但类型未知
	 * \return FileSystemError::Success 查询成功
	 *         FileSystemError::NotFound 路径不存在
	 *         FileSystemError::Invaild 无效路径
	 *         FileSystemError::AccessDenied 程序权限不足
	 */
	virtual FileSystemError getFileType(const std::string& path, FileType& outType) = 0;

	/**
	 * \brief 获取文件大小（字节）
	 * 
	 * \param path 文件路径
	 * \param[out] outSize 输出参数，返回文件大小（字节）
	 * \return FileSystemError::Success 获取成功
	 *         FileSystemError::NotFound 文件不存在
	 *         FileSystemError::IsDirectory 路径为目录
	 *         FileSystemError::Invaild 无效路径
	 *         FileSystemError::AccessDenied 程序权限不足
	 */
	virtual FileSystemError getFileSize(const std::string& path, uint64_t& outSize) = 0;

	/**
	 * \brief 获取文件的最后修改时间（毫秒时间戳）
	 * 
	 * \param path 文件路径
	 * \param[out] outTime 输出参数，返回时间戳
	 * \return FileSystemError::Success 获取成功
	 *         FileSystemError::NotFound 文件不存在
	 *         FileSystemError::IsDirectory 路径为目录
	 *         FileSystemError::Invaild 无效路径
	 *         FileSystemError::AccessDenied 程序权限不足
	 */
	virtual FileSystemError getLastModifiedTime(const std::string& path, int64_t& outTime) = 0;

protected:
	std::string error_message_ = { 0 };
	// 设置错误消息
	void setErrorMessage(const std::string& msg) {
		error_message_ = msg;
	}
	// 清除错误消息
	void clearErrorMessage() {
		error_message_.clear();
	}
};