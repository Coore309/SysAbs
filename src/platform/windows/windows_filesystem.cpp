#include <windows_filesystem.h>
#include <Windows.h>

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

// 文件属性获取 Functor
class FileAttributesGetter {
public:
	// 属性数据
	_WIN32_FILE_ATTRIBUTE_DATA attrData_ = { 0 };
	// 错误码
	DWORD err_ = 0;
	// 错误类型
	FileSystemError errType = FileSystemError::UnKnown;
	// 详细错误信息
	std::string error_message_;

	FileAttributesGetter() = default;

	FileAttributesGetter(const std::string& path) {
		errType = execute(path);
	}

	~FileAttributesGetter() = default;

	FileSystemError operator()(const std::string& path) {
		return errType = execute(path);
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

			if (err_ == ERROR_INVALID_NAME || err_ == ERROR_BAD_PATHNAME) {
				error_message_ = "路径包含非法字符或格式错误：" + path;
				return FileSystemError::InvalidPath;
			}

			// 路径不存在
			if (err_ == ERROR_FILE_NOT_FOUND || err_ == ERROR_PATH_NOT_FOUND) {
				error_message_ = {};
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
		error_message_ = {};
		return FileSystemError::Success;
	}
};


FileSystemError WindowsFileSystem::exists(const std::string& path) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);
	if (attrGetter.errType != FileSystemError::Success && attrGetter.errType != FileSystemError::NotFound) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);

		return attrGetter.errType;
	}

	return attrGetter.errType;
}

FileSystemError WindowsFileSystem::getFileType(const std::string& path, FileType& outType) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);

	if (attrGetter.errType == FileSystemError::NotFound) {
		outType = FileType::NotFound;
		return attrGetter.errType;
	}

	if (attrGetter.errType != FileSystemError::Success) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);
		outType = FileType::Unknown;

		return attrGetter.errType;
	}

	if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		outType = FileType::Directory;
	}
	else {
		outType = FileType::File;
	}

	return attrGetter.errType;
}

FileSystemError WindowsFileSystem::getFileSize(const std::string& path, uint64_t& outSize) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);
	if (attrGetter.errType != FileSystemError::Success && attrGetter.errType != FileSystemError::NotFound) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);

		return attrGetter.errType;
	}

	if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FileSystemError::IsDirectory;
	outSize = (static_cast<uint64_t>(attrGetter.attrData_.nFileSizeHigh) << 32) | attrGetter.attrData_.nFileSizeLow;

	return attrGetter.errType;
}

FileSystemError WindowsFileSystem::getLastModifiedTime(const std::string& path, int64_t& outTime) {
	FileAttributesGetter attrGetter;
	clearErrorMessage();
	attrGetter(path);
	if (attrGetter.errType != FileSystemError::Success && attrGetter.errType != FileSystemError::NotFound) {
		if (!attrGetter.error_message_.empty())
			setErrorMessage(attrGetter.error_message_);

		return attrGetter.errType;
	}

	if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FileSystemError::IsDirectory;
	uint64_t fileTime = (static_cast<uint64_t>(attrGetter.attrData_.ftLastWriteTime.dwHighDateTime) << 32) | attrGetter.attrData_.ftLastWriteTime.dwLowDateTime;
	const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL;
	outTime = static_cast<int64_t>((fileTime - EPOCH_DIFFERENCE) / 10000);

	return attrGetter.errType;
}