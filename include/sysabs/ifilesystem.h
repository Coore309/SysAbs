#pragma once

#include <string>
#include <memory>
#include <vector>

namespace sysabs {
	/**
	 * \brief 文件系统操作错误码枚举
	 *
	 * 所有 IFileSystem 方法均返回该枚举，表示简单表示操作成功或失败抑或是失败的大致原因。详细错误信息请使用 getLastErrorMessage() 方法获取。
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
		// 路径为文件（期望为目录时）
		IsFile,
		// 磁盘空间不足
		DiskFull,
		// 文件只读
		WriteProtected,
		// 共享冲突，文件被其他程序占用
		SharingViolation,
		// 目标（文件或目录）已存在
		AlreadyExists,
		// 目录非空
		DirectoryNotEmpty,
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

	struct DirectoryEntry {
		std::string name;
		FileType type = FileType::Unknown;
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
		virtual std::string getLastErrorMessage() = 0;

		// 检查路径是否有效，能在当前系统被解析
		virtual bool validatePath(const std::string& path) = 0;

		/*--------------------------------------------------------------------------
		* 
		* 文件存在性及其元数据查询
		* 
		---------------------------------------------------------------------------*///

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
		 *         FileSystemError::UnKnown 未知错误
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
		 *         FileSystemError::UnKnown 未知错误
		 */
		virtual FileSystemError getLastModifiedTime(const std::string& path, int64_t& outTime) = 0;


		/*--------------------------------------------------------------------------
		* 
		* 文件二进制读写
		* 
		---------------------------------------------------------------------------*///

		/**
		 * \brief 以二进制形式读取文件所有内容
		 * 
		 * \param path 文件路径
		 * \param outData 输出参数，返回二进制数据
		 * \return FileSystemError::Success 读取成功
		 *         FileSystemError::NotFound 文件不存在
		 *         FileSystemError::IsDirectory 路径为目录
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::Invaild 无效路径
		 *         FileSystemError::UnKnown 未知错误
		 */
		virtual FileSystemError readAllBytes(const std::string& path, std::vector<uint8_t>& outData) = 0;

		/**
		 * \brief 以二进制形式写入文件所有内容（覆盖已有内容，若文件不存在则创建文件）
		 * 
		 * \param path 文件路径
		 * \param data 要写入的二进制数据
		 * \return FileSystemError::Success 写入成功
		 *         FileSystemError::NotFound 父目录不存在
		 *         FileSystemError::IsDirectory 路径为目录
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::Invaild 无效路径
		 *         FileSystemError::DiskFull 磁盘空间不足
		 *         FileSystemError::WriteProtected 文件只读
		 *         FileSystemError::UnKnown 未知错误
		 */
		virtual FileSystemError writeAllBytes(const std::string& path, std::vector<uint8_t>& data) = 0;

		/**
		 * \brief 以二进制形式追加数据到文件末尾（若文件不存在则创建文件）
		 * 
		 * \param path 文件路径
		 * \param data 要追加的二进制数据
		 * \return FileSystemError::Success 写入成功
		 *         FileSystemError::NotFound 父目录不存在
		 *         FileSystemError::IsDirectory 路径为目录
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::Invaild 无效路径
		 *         FileSystemError::DiskFull 磁盘空间不足
		 *         FileSystemError::WriteProtected 文件只读
		 *         FileSystemError::UnKnown 未知错误
		 */
		virtual FileSystemError appendAllBytes(const std::string& path, std::vector<uint8_t>& data) = 0;


		/*--------------------------------------------------------------------------
		*
		* 文件与目录操作
		*
		---------------------------------------------------------------------------*///

		/**
		 * \brief 列出指定目录下的所有文件和子目录
		 *
		 * \param path 目录路径
		 * \param[out] outEntries 输出参数，返回目录中的所有条目
		 * \return FileSystemError::Success 遍历成功
		 *         FileSystemError::NotFound 目录不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::IsFile 路径为文件
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError listDirectory(const std::string& path, std::vector<DirectoryEntry>& outEntries) = 0;

		/**
		 * \brief 移动文件
		 *
		 * 将文件从源路径移动到目标路径，支持跨盘
		 * 若目标已存在且 overwrite 为 false，则返回 FileSystemError::AlreadyExists
		 * 在某些平台下，moveFile 对目录也能操作成功并且不返回 FileSystemError::IsDirectory，但是强烈建议分开使用
		 *
		 * \param src 源文件路径
		 * \param dst 目标文件路径
		 * \param overwrite 是否覆盖已有文件（默认为 false）
		 * \return FileSystemError::Success 移动成功
		 *         FileSystemError::NotFound 源文件不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::IsDirectory 源路径指向目录，若需要移动目录，请调用 moveDirectory 函数
		 *         FileSystemError::AlreadyExists 目标已存在且本次操作为不覆盖
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError moveFile(const std::string& src, const std::string& dst, bool overwrite = false) = 0;

		/**
		 * \brief 复制文件
		 *
		 * 将文件从源路径复制到目标路径，支持跨盘
		 * 若目标已存在且 overwrite 为 false，则返回 FileSystemError::AlreadyExists
		 *
		 * \param src 源文件路径
		 * \param dst 目标文件路径
		 * \param overwrite 是否覆盖已有文件（默认为 false）
		 * \return FileSystemError::Success 移动成功
		 *         FileSystemError::NotFound 源文件不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::IsDirectory 源路径指向目录，若需要复制目录，请调用 copyDirectory 函数
		 *         FileSystemError::DiskFull 磁盘空间不足
		 *         FileSystemError::AlreadyExists 目标已存在且本次操作为不覆盖
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError copyFile(const std::string& src, const std::string& dst, bool overwrite = false) = 0;

		/**
		 * \brief 删除文件
		 * 
		 * \param path 要删除的文件路径
		 * \return FileSystemError::Success 删除成功
		 *         FileSystemError::NotFound 文件不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::IsDirectory 源路径指向目录，若需要删除目录，请调用 removeDirectory 函数
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError deleteFile(const std::string& path) = 0;

		/**
		 * \brief 创建目录
		 * 
		 * 在指定路径创建目录，只创建单层，不支持递归创建
		 * 若路径已存在同名文件，返回 FileSystemError::IsFile
		 * 若目录已存在，返回 FileSystemError::AlreadyExists
		 * 
		 * \param path 要创建的目录路径
		 * \return FileSystemError::Success 创建成功
		 *         FileSystemError::NotFound 父目录不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::AlreadyExists 目录已存在
		 *         FileSystemError::IsFile 路径已存在同名文件
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError createDirectory(const std::string& path) = 0;

		/**
		 * \brief 移动目录
		 * 
		 * \param src 源目录路径
		 * \param dst 目标目录路径
		 * \param overwrite 是否覆盖已有目录（默认为 false）
		 * \return FileSystemError::Success 移动成功
		 *         FileSystemError::NotFound 源目录不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::AlreadyExists 目录已存在且本次操作为不覆盖
		 *         FileSystemError::DirectoryNotEmpty 目录已存在且非空，无法覆盖
		 *         FileSystemError::IsFile 源路径为文件，若需要移动文件，请调用 moveFile 函数
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError moveDirectory(const std::string& src, const std::string& dst, bool overwrite = false) = 0;

		/**
		 * \brief 删除目录
		 * 
		 * \param path 要删除的目录路径
		 * \return FileSystemError::Success 删除成功
		 *         FileSystemError::NotFound 目录不存在
		 *         FileSystemError::InvalidPath 无效路径
		 *         FileSystemError::AccessDenied 程序权限不足
		 *         FileSystemError::DirectoryNotEmpty 目录非空且本次操作为不递归删除
		 *         FileSystemError::IsFile 路径为文件，若需要删除文件，请调用 deleteFile 函数
		 *         FileSystemError::Unknown 未知错误
		 */
		virtual FileSystemError removeDirectory(const std::string& path) = 0;
	};

	/**
	 * \brief 创建一个 IFileSystem 实例
	 */
	std::unique_ptr<IFileSystem> createFileSystem();
}
