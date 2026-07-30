#include <sysabs/platform/windows_filesystem.h>
#include <Windows.h>

/*--------------------------------------------------------------------------
* 
* 工具函数、基类
* 
---------------------------------------------------------------------------*///

//char转化为宽字符
std::wstring ChartoWide(const std::string& utf8) {
	if (utf8.empty()) return {};
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
	if (len <= 0) return {};
	std::wstring wstr(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
	wstr.pop_back();
	return wstr;
}

// 带错误信息的 Functor 基类
class FunctorBase {
public:
	// 错误码
	DWORD err_ = 0;
	// 错误类型
	FileSystemError errType_ = FileSystemError::UnKnown;
	// 详细错误信息
	std::string error_message_;
};


/*--------------------------------------------------------------------------
* 
* 文件存在性及其元数据查询
* 
---------------------------------------------------------------------------*///

// 文件属性获取器 Functor
class FileAttributesGetter: public FunctorBase {
public:
	// 属性数据
	_WIN32_FILE_ATTRIBUTE_DATA attrData_ = { 0 };

	FileAttributesGetter() = default;

	FileAttributesGetter(const std::string& path) {
		errType_ = execute(path);
	}

	~FileAttributesGetter() = default;

	FileSystemError operator()(const std::string& path) {
		return errType_ = execute(path);
	}

private:
	FileSystemError execute(const std::string& path) {
		// 检查路径名是否为空
		if (path.empty()) {
			error_message_ = "路径为空";
			err_ = ERROR_INVALID_PARAMETER;
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wpath = ChartoWide(path);
		if (wpath.empty() && !path.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			err_ = ERROR_INVALID_PARAMETER;
			return FileSystemError::InvalidPath;
		}

		// 调用API
		if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &attrData_)) {
			// 处理错误
			err_ = GetLastError();

			// 非法路径
			if (err_ == ERROR_INVALID_NAME || err_ == ERROR_BAD_PATHNAME) {
				error_message_ = "路径包含非法字符或格式错误：" + path;
				return FileSystemError::InvalidPath;
			}

			// 路径不存在
			if (err_ == ERROR_FILE_NOT_FOUND || err_ == ERROR_PATH_NOT_FOUND) {
				error_message_.clear();
				return FileSystemError::NotFound;
			}

			// 权限不足
			if (err_ == ERROR_ACCESS_DENIED) {
				error_message_ = "拒绝访问：" + path;
				return FileSystemError::AccessDenied;
			}

			// 其他错误
			error_message_ = "GetFileAttributes 发生错误！错误码：" + std::to_string(err_);
			return FileSystemError::UnKnown;
		}

		// 成功
		err_ = 0;
		error_message_.clear();
		return FileSystemError::Success;
	}
};

FileSystemError WindowsFileSystem::exists(const std::string& path) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);
	if (attrGetter.errType_ != FileSystemError::Success && attrGetter.errType_ != FileSystemError::NotFound) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);

		return attrGetter.errType_;
	}

	return attrGetter.errType_;
}

FileSystemError WindowsFileSystem::getFileType(const std::string& path, FileType& outType) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);

	if (attrGetter.errType_ == FileSystemError::NotFound) {
		outType = FileType::NotFound;
		return attrGetter.errType_;
	}

	if (attrGetter.errType_ != FileSystemError::Success) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);
		outType = FileType::Unknown;

		return attrGetter.errType_;
	}

	if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		outType = FileType::Directory;
	}
	else {
		outType = FileType::File;
	}

	return attrGetter.errType_;
}

FileSystemError WindowsFileSystem::getFileSize(const std::string& path, uint64_t& outSize) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);
	if (attrGetter.errType_ != FileSystemError::Success && attrGetter.errType_ != FileSystemError::NotFound) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);

		return attrGetter.errType_;
	}

	if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FileSystemError::IsDirectory;
	outSize = (static_cast<uint64_t>(attrGetter.attrData_.nFileSizeHigh) << 32) | attrGetter.attrData_.nFileSizeLow;

	return attrGetter.errType_;
}

FileSystemError WindowsFileSystem::getLastModifiedTime(const std::string& path, int64_t& outTime) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);
	if (attrGetter.errType_ != FileSystemError::Success && attrGetter.errType_ != FileSystemError::NotFound) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);

		return attrGetter.errType_;
	}

	if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FileSystemError::IsDirectory;
	uint64_t fileTime = (static_cast<uint64_t>(attrGetter.attrData_.ftLastWriteTime.dwHighDateTime) << 32) | attrGetter.attrData_.ftLastWriteTime.dwLowDateTime;
	const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL;
	outTime = static_cast<int64_t>((fileTime - EPOCH_DIFFERENCE) / 10000);

	return attrGetter.errType_;
}


/*--------------------------------------------------------------------------
* 
* 文件二进制读写
* 
---------------------------------------------------------------------------*///

// 智能文件句柄 Functor
class FileHandler : public FunctorBase {
public:
	FileHandler(const std::string& path, DWORD accessMode, DWORD shareMode, DWORD creationDisposition) {
		errType_ = CreateHandle(path, accessMode, shareMode, creationDisposition);
	}
	~FileHandler() {
		if (isValid()) {
			CloseHandle(file_handle_);
			file_handle_ = INVALID_HANDLE_VALUE;
		}
	}
	
	// 获取文件句柄
	HANDLE getHandle(void) {
		return file_handle_;
	}

	// 检查句柄是否有效
	bool isValid(void) {
		return file_handle_ != INVALID_HANDLE_VALUE;
	}
private:
	// 文件句柄
	HANDLE file_handle_ = INVALID_HANDLE_VALUE;
	// 创建句柄
	FileSystemError CreateHandle(const std::string& path, DWORD accessMode, DWORD shareMode, DWORD creationDisposition) {
		// 检查路径名是否为空
		if (path.empty()) {
			error_message_ = "路径为空";
			err_ = ERROR_INVALID_PARAMETER;
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wpath = ChartoWide(path);
		if (wpath.empty() && !path.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			err_ = ERROR_INVALID_PARAMETER;
			return FileSystemError::InvalidPath;
		}

		// 获取文件属性
		FileAttributesGetter attrGetter(path);
		// 错误检查
		if (attrGetter.errType_ != FileSystemError::Success) {
			if (!((attrGetter.errType_ == FileSystemError::NotFound) && (creationDisposition != OPEN_EXISTING))) {
				if (!attrGetter.error_message_.empty())
					error_message_ = attrGetter.error_message_;
				return attrGetter.errType_;
			}
		}
		
		// 检查是否为目录
		if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			error_message_ = "路径为目录，无法进行文件操作";
			return FileSystemError::IsDirectory;
		}

		// 在写操作时，检查是否为只读文件
		if (accessMode & (GENERIC_WRITE | FILE_APPEND_DATA)) {
			if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
				error_message_ = "文件只读，无法写入：" + path;
				return FileSystemError::WriteProtected;
			}
		}

		// 调用API
		file_handle_ = CreateFileW(
			wpath.c_str(),
			accessMode,
			shareMode,
			NULL,
			creationDisposition,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		// 错误处理
		if (file_handle_ == INVALID_HANDLE_VALUE) {
			err_ = GetLastError();

			// 非法路径
			if (err_ == ERROR_INVALID_NAME || err_ == ERROR_BAD_PATHNAME) {
				error_message_ = "路径包含非法字符或格式错误：" + path;
				return FileSystemError::InvalidPath;
			}

			// 目录不存在
			if (err_ == ERROR_FILE_NOT_FOUND || err_ == ERROR_PATH_NOT_FOUND) {
				if (creationDisposition == OPEN_EXISTING) error_message_ = "文件或路径不存在：" + path;
				else error_message_ = "父目录不存在，无法创建文件：" + path;
				return FileSystemError::NotFound;
			}

			// 权限不足
			if (err_ == ERROR_ACCESS_DENIED) {
				error_message_ = "拒绝访问：" + path;
				return FileSystemError::AccessDenied;
			}

			// 共享冲突
			if (err_ == ERROR_SHARING_VIOLATION) {
				error_message_ = "文件被其他进程占用，无法访问：" + path;
				return FileSystemError::SharingViolation;
			}

			// 其他错误
			error_message_ = "CreateFileW 创建句柄失败！错误码：" + std::to_string(err_);
			return FileSystemError::UnKnown;
		}

		// 成功
		return FileSystemError::Success;
	}
};

FileSystemError WindowsFileSystem::readAllBytes(const std::string& path, std::vector<uint8_t>& outData) {
	clearErrorMessage();

	// 获取文件句柄
	FileHandler handle(path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
	if (!handle.isValid()) {
		setErrorMessage(handle.error_message_);
		return handle.errType_;
	}

	// 获取文件大小
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(handle.getHandle(), &fileSize)) {
		setErrorMessage("获取文件大小失败");
		return FileSystemError::UnKnown;
	}

	// 分配缓冲区并开始读取
	size_t size = static_cast<size_t>(fileSize.QuadPart);
	outData.resize(size);
	if (size > 0) {
		DWORD bytesRead = 0;
		if (!ReadFile(handle.getHandle(), outData.data(), static_cast<DWORD>(size), &bytesRead, nullptr) || (bytesRead != size)) {
			setErrorMessage("读取文件内容失败");
			return FileSystemError::UnKnown;
		}
	}

	return FileSystemError::Success;
}

class FileWriter : public FunctorBase {
public:
	FileWriter() = default;
	FileWriter(const std::string& path, const std::vector<uint8_t>& data, bool append) {
		errType_ = execute(path, data, append);
	}
	~FileWriter() = default;

	FileSystemError operator()(const std::string& path, const std::vector<uint8_t>& data, bool append) {
		return errType_ = execute(path, data, append);
	}

private:
	FileSystemError execute(const std::string& path, const std::vector<uint8_t>& data, bool append) {
		// 获取文件句柄
		FileHandler handle(path, append ? FILE_APPEND_DATA : GENERIC_WRITE, append ? FILE_SHARE_READ : 0, append ? OPEN_ALWAYS : CREATE_ALWAYS);
		if (!handle.isValid()) {
			error_message_ = handle.error_message_;
			return handle.errType_;
		}

		// 写入数据
		DWORD bytesWritten = 0;
		if (!WriteFile(handle.getHandle(), data.data(), static_cast<DWORD>(data.size()), &bytesWritten, nullptr)) {
			err_ = GetLastError();

			if (err_ == ERROR_DISK_FULL) {
				error_message_ = "磁盘空间不足，无法写入：" + path;
				return FileSystemError::DiskFull;
			}

			if (err_ == ERROR_WRITE_PROTECT) {
				error_message_ = "磁盘写保护，无法写入：" + path;
				return FileSystemError::WriteProtected;
			}

			error_message_ = "写入文件失败，错误码：" + std::to_string(err_);
			return FileSystemError::UnKnown;
		}

		// 特判字节数
		if (bytesWritten != data.size()) {
			error_message_ = "写入字节数不匹配，可能磁盘已满";
			return FileSystemError::UnKnown;
		}

		return FileSystemError::Success;
	}
};

FileSystemError WindowsFileSystem::writeAllBytes(const std::string& path, std::vector<uint8_t>& data) {
	clearErrorMessage();

	FileWriter writeFile(path, data, false);
	if (writeFile.errType_ != FileSystemError::Success) {
		setErrorMessage(writeFile.error_message_);
		return writeFile.errType_;
	}

	return FileSystemError::Success;
}

FileSystemError WindowsFileSystem::appendAllBytes(const std::string& path, std::vector<uint8_t>& data){
	clearErrorMessage();

	FileWriter writeFile(path, data, true);
	if (writeFile.errType_ != FileSystemError::Success) {
		setErrorMessage(writeFile.error_message_);
		return writeFile.errType_;
	}

	return FileSystemError::Success;
}
