#pragma once

#include <sysabs/ifilesystem.h>
using namespace sysabs;

class WindowsFileSystem : public IFileSystem {
public:
    ~WindowsFileSystem() = default;

	// 获取最后一次错误消息
	std::string getLastErrorMessage() {
		return error_message_;
	}

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
    FileSystemError exists(const std::string& path) override;

	/**
	* @brief 获取路径对于的类型
	*
	* @param path 路径
	* @param[out] outType 输出参数类型为 FileType ，返回以下结果：
	*						- FileType::File: 路径为文件
	*						- FileType::Directory: 路径为目录
	*						- FileType::NotFound: 路径不存在
	*						- FileType::Unknown: 路径存在但类型未知
	* @return FileSystemError::Success 查询成功
	*         FileSystemError::NotFound 路径不存在
	*         FileSystemError::Invaild 无效路径
	*         FileSystemError::AccessDenied 程序权限不足
	*/
    FileSystemError getFileType(const std::string& path, FileType& outType) override;

	/**
	* @brief 获取文件大小（字节）
	*
	* @param path 文件路径
	* @param[out] outSize 输出参数，返回文件大小（字节）
	* @return FileSystemError::Success 获取成功
	*         FileSystemError::NotFound 文件不存在
	*         FileSystemError::IsDirectory 路径为目录
	*         FileSystemError::Invaild 无效路径
	*         FileSystemError::AccessDenied 程序权限不足
	*/
    FileSystemError getFileSize(const std::string& path, uint64_t& outSize) override;

	/**
	* @brief 获取文件的最后修改时间（毫秒时间戳）
	*
	* @param path 文件路径
	* @param[out] outTime 输出参数，返回时间戳
	* @return FileSystemError::Success 获取成功
	*         FileSystemError::NotFound 文件不存在
	*         FileSystemError::IsDirectory 路径为目录
	*         FileSystemError::Invaild 无效路径
	*         FileSystemError::AccessDenied 程序权限不足
	*/
    FileSystemError getLastModifiedTime(const std::string& path, int64_t& outTime) override;


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
	FileSystemError readAllBytes(const std::string& path, std::vector<uint8_t>& outData) override;

	/**
	 * \brief 以二进制形式读取文件所有内容（覆盖已有内容，若文件不存在则创建文件）
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
	FileSystemError writeAllBytes(const std::string& path, std::vector<uint8_t>& data) override;

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
	FileSystemError appendAllBytes(const std::string& path, std::vector<uint8_t>& data) override;

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