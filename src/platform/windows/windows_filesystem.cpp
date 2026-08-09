#include <sysabs/platform/windows_filesystem.h>
#include <Windows.h>

namespace sysabs {
	/*--------------------------------------------------------------------------
*
* 工具函数、基类
*
---------------------------------------------------------------------------*///

//char转化为宽字符，只用于路径字符转换
	std::wstring ChartoWide(const std::string& utf8) {
		if (utf8.empty()) return {};
		std::string nmstr = utf8;
		for (auto& ch : nmstr)
			if (ch == L'/') ch = L'\\';

		int len = MultiByteToWideChar(CP_UTF8, 0, nmstr.c_str(), -1, nullptr, 0);
		if (len <= 0) return {};
		std::wstring wstr(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, nmstr.c_str(), -1, &wstr[0], len);
		wstr.pop_back();
		return wstr;
	}

	//宽字符转化为char
	std::string WidetoChar(const std::wstring& wstr) {
		if (wstr.empty()) return {};
		int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return {};
		std::string str(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
		str.pop_back();
		return str;
	}

	bool WindowsFileSystem::validatePath(const std::string& path) {
		if (path.empty()) return false;
		if (path.length() >= MAX_PATH) return false;

		//逐字检查
		for (size_t i = 0; i < path.length(); ++i) {
			unsigned char ch = static_cast<unsigned char>(path[i]);

			if (ch < 32) return false;
			if (ch == 127) return false;
			switch (ch) {
			case '<': case '>': case '"':
			case '|': case '?': case '*':
				return false;
			case ':':
				// 验证盘符
				if (i != 1) return false;
				if (!((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z'))) return false;
				break;
			default:
				break;
			}
		}

		return true;
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
	class FileAttributesGetter : public FunctorBase {
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

				// 文件不存在
				if (err_ == ERROR_FILE_NOT_FOUND) {
					error_message_ = "目标不存在：" + path;
					return FileSystemError::NotFound;
				}

				// 路径不存在
				if (err_ == ERROR_PATH_NOT_FOUND) {
					error_message_ = "路径不存在" + path;
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
		clearErrorMessage();

		FileAttributesGetter attrGetter;
		attrGetter(path);
		if (attrGetter.errType_ != FileSystemError::Success) {
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
			setErrorMessage(attrGetter.error_message_);
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
		if (attrGetter.errType_ != FileSystemError::Success) {
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
		if (attrGetter.errType_ != FileSystemError::Success) {
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
					else error_message_ = "目录不存在，无法创建文件：" + path;
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

	FileSystemError WindowsFileSystem::appendAllBytes(const std::string& path, std::vector<uint8_t>& data) {
		clearErrorMessage();

		FileWriter writeFile(path, data, true);
		if (writeFile.errType_ != FileSystemError::Success) {
			setErrorMessage(writeFile.error_message_);
			return writeFile.errType_;
		}

		return FileSystemError::Success;
	}


	/*--------------------------------------------------------------------------
	*
	* 文件与目录操作
	*
	---------------------------------------------------------------------------*///

	FileSystemError WindowsFileSystem::listDirectory(const std::string& path, std::vector<DirectoryEntry>& outEntries) {
		clearErrorMessage();
		outEntries.clear();

		// 检查路径名是否为空
		if (path.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wpath = ChartoWide(path);
		if (wpath.empty() && !path.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		// 获取路径对象属性并检查路径是否为目录（期望目录，拿到文件则返回 IsFile）
		FileAttributesGetter attrGetter(path);
		if (attrGetter.errType_ != FileSystemError::Success) {
			if (!attrGetter.error_message_.empty())
				setErrorMessage(attrGetter.error_message_);
			return attrGetter.errType_;
		}

		if (!(attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			setErrorMessage("路径为文件，无法列出目录内容：" + path);
			return FileSystemError::IsFile;
		}

		// 路径标准化、装配通配符
		std::wstring searchPath = wpath;
		if (searchPath.back() != L'\\')
			searchPath += L'\\';
		searchPath += L'*';

		// 打开迭代器，获取第一个条目
		WIN32_FIND_DATAW findData;
		HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
		if (hFind == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError();

			if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
				setErrorMessage("目录不存在：" + path);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问：" + path);
				return FileSystemError::AccessDenied;
			}

			if (err == ERROR_INVALID_NAME || err == ERROR_BAD_PATHNAME) {
				setErrorMessage("路径包含非法字符：" + path);
				return FileSystemError::InvalidPath;
			}

			setErrorMessage("列出目录失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		// 循环遍历迭代器
		do {
			if (wcscmp(findData.cFileName, L".") == 0) continue;
			if (wcscmp(findData.cFileName, L"..") == 0) continue;

			DirectoryEntry entry;
			entry.name = WidetoChar(findData.cFileName);
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) entry.type = FileType::Directory;
			else entry.type = FileType::File;

			outEntries.push_back(std::move(entry));
		} while (FindNextFileW(hFind, &findData));

		// 判断遍历结束原因
		DWORD endErr = GetLastError();
		if (endErr != ERROR_NO_MORE_FILES) {
			FindClose(hFind);
			setErrorMessage("遍历目录时出错，错误码：" + std::to_string(endErr));
			return FileSystemError::UnKnown;
		}

		FindClose(hFind);
		return FileSystemError::Success;
	}

	FileSystemError WindowsFileSystem::moveFile(const std::string& src, const std::string& dst, bool overwrite) {
		// 检查路径名是否为空
		if (src.empty() || dst.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wsrc = ChartoWide(src);
		std::wstring wdst = ChartoWide(dst);
		if (wsrc.empty() || wdst.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		// 获取源路径对象属性并检查路径是否为目录（期望文件，拿到目录则返回 IsDirectory）
		FileAttributesGetter attrGetter(src);
		if (attrGetter.errType_ != FileSystemError::Success) {
			if (!attrGetter.error_message_.empty())
				setErrorMessage(attrGetter.error_message_);
			return attrGetter.errType_;
		}

		if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			setErrorMessage("路径为目录，请使用 moveDirectory 移动目录：" + src);
			return FileSystemError::IsDirectory;
		}

		if (!MoveFileExW(wsrc.c_str(), wdst.c_str(), MOVEFILE_COPY_ALLOWED | (overwrite ? MOVEFILE_REPLACE_EXISTING : 0))) {
			DWORD err = GetLastError();
			if (err == ERROR_FILE_NOT_FOUND) {
				setErrorMessage("源文件不存在：" + src + "->" + dst);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_PATH_NOT_FOUND) {
				setErrorMessage("源路径或目标路径目录不存在：" + src + "->" + dst);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问：" + src);
				return FileSystemError::AccessDenied;
			}

			if (err == ERROR_INVALID_NAME || err == ERROR_BAD_PATHNAME) {
				setErrorMessage("路径包含非法字符");
				return FileSystemError::InvalidPath;
			}

			if (err == ERROR_ALREADY_EXISTS) {
				setErrorMessage("目标已存在：" + dst);
				return FileSystemError::AlreadyExists;
			}

			setErrorMessage("移动文件失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		return FileSystemError::Success;
	}

	FileSystemError WindowsFileSystem::copyFile(const std::string& src, const std::string& dst, bool overwrite) {
		clearErrorMessage();

		// 检查路径名是否为空
		if (src.empty() || dst.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wsrc = ChartoWide(src);
		std::wstring wdst = ChartoWide(dst);
		if (wsrc.empty() || wdst.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		// 获取源路径对象属性并检查路径是否为目录（期望文件，拿到目录则返回 IsDirectory）
		FileAttributesGetter attrGetter(src);
		if (attrGetter.errType_ != FileSystemError::Success) {
			if (!attrGetter.error_message_.empty())
				setErrorMessage(attrGetter.error_message_);
			return attrGetter.errType_;
		}

		if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			setErrorMessage("路径为目录，请使用 moveDirectory 移动目录：" + src);
			return FileSystemError::IsDirectory;
		}

		if (!CopyFileW(wsrc.c_str(), wdst.c_str(), overwrite ? FALSE : TRUE)) {
			DWORD err = GetLastError();

			if (err == ERROR_FILE_NOT_FOUND) {
				setErrorMessage("源文件不存在：" + src + "->" + dst);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_PATH_NOT_FOUND) {
				setErrorMessage("源路径或目标路径目录不存在：" + src + "->" + dst);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问");
				return FileSystemError::AccessDenied;
			}

			if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) {
				setErrorMessage("目标已存在：" + dst);
				return FileSystemError::AlreadyExists;
			}

			if (err == ERROR_HANDLE_DISK_FULL || err == ERROR_DISK_FULL) {
				setErrorMessage("磁盘空间不足");
				return FileSystemError::DiskFull;
			}

			setErrorMessage("复制文件失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		clearErrorMessage();
		return FileSystemError::Success;
	}

	FileSystemError WindowsFileSystem::deleteFile(const std::string& path) {
		clearErrorMessage();

		// 检查路径名是否为空
		if (path.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wpath = ChartoWide(path);
		if (wpath.empty() && !path.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		// 获取路径对象属性并检查路径是否为目录（期望目录，拿到文件则返回 IsFile）
		FileAttributesGetter attrGetter(path);
		if (attrGetter.errType_ != FileSystemError::Success) {
			if (!attrGetter.error_message_.empty())
				setErrorMessage(attrGetter.error_message_);
			return attrGetter.errType_;
		}

		if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			setErrorMessage("路径为目录，请使用 removeDirectory 函数删除目录：" + path);
			return FileSystemError::IsDirectory;
		}

		if (!DeleteFileW(wpath.c_str())) {
			DWORD err = GetLastError();

			if (err == ERROR_FILE_NOT_FOUND) {
				setErrorMessage("源文件不存在：" + path);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_PATH_NOT_FOUND) {
				setErrorMessage("源路径不存在：" + path);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问：" + path);
				return FileSystemError::AccessDenied;
			}

			setErrorMessage("删除文件失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		clearErrorMessage();
		return FileSystemError::Success;
	}

	FileSystemError WindowsFileSystem::createDirectory(const std::string& path) {
		clearErrorMessage();

		// 检查路径名是否为空
		if (path.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		// 转换字符串
		std::wstring wpath = ChartoWide(path);
		if (wpath.empty() && !path.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		// 获取文件属性
		FileAttributesGetter attrGetter(path);
		if (attrGetter.errType_ == FileSystemError::Success) {
			// 路径已存在，判断类型
			if (attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				setErrorMessage("目录已存在：" + path);
				return FileSystemError::AlreadyExists;
			}
			else {
				setErrorMessage("路径已存在同名文件：" + path);
				return FileSystemError::IsFile;
			}
		}
		else if (attrGetter.errType_ != FileSystemError::NotFound) {
			if (!attrGetter.error_message_.empty())
				setErrorMessage(attrGetter.error_message_);
			return attrGetter.errType_;
		}

		if (!CreateDirectoryW(wpath.c_str(), NULL)) {
			DWORD err = GetLastError();

			if (err == ERROR_PATH_NOT_FOUND) {
				setErrorMessage("父目录不存在，无法创建目录：" + path);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问：" + path);
				return FileSystemError::AccessDenied;
			}

			setErrorMessage("创建目录失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		return FileSystemError::Success;
	}

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
	FileSystemError WindowsFileSystem::moveDirectory(const std::string& src, const std::string& dst, bool overwrite) {
		clearErrorMessage();

		if (src.empty() || dst.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		std::wstring wsrc = ChartoWide(src);
		std::wstring wdst = ChartoWide(dst);
		if (wsrc.empty() || wdst.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		FileAttributesGetter attrGetter_S(src);
		if (attrGetter_S.errType_ != FileSystemError::Success) {
			if (!attrGetter_S.error_message_.empty())
				setErrorMessage(attrGetter_S.error_message_);
			return attrGetter_S.errType_;
		}

		if (!(attrGetter_S.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			setErrorMessage("路径为文件，请使用 moveFile 移动文件：" + src);
			return FileSystemError::IsFile;
		}

		// 检查目标路径
		FileAttributesGetter attrGetter_D(dst);
		if (attrGetter_D.errType_ == FileSystemError::Success) {
			// 目标已存在
			if (!(attrGetter_D.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				setErrorMessage("目标路径为文件，无法移动目录到此路径：" + dst);
				return FileSystemError::IsFile;
			}
			if (!overwrite) {
				setErrorMessage("目标目录已存在：" + dst);
				return FileSystemError::AlreadyExists;
			}
		}
		else if (attrGetter_D.errType_ != FileSystemError::NotFound) {
			if (!attrGetter_D.error_message_.empty())
				setErrorMessage(attrGetter_D.error_message_);
			return attrGetter_D.errType_;
		}

		if (!MoveFileExW(wsrc.c_str(), wdst.c_str(), MOVEFILE_COPY_ALLOWED | (overwrite ? MOVEFILE_REPLACE_EXISTING : 0))) {
			DWORD err = GetLastError();
			if (err == ERROR_FILE_NOT_FOUND) {
				setErrorMessage("源目录不存在：" + src + "->" + dst);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_PATH_NOT_FOUND) {
				setErrorMessage("源路径或目标路径目录不存在：" + src + "->" + dst);
				return FileSystemError::NotFound;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问：" + src);
				return FileSystemError::AccessDenied;
			}

			if (err == ERROR_ALREADY_EXISTS) {
				setErrorMessage("目标目录已存在：" + dst);
				return FileSystemError::AlreadyExists;
			}

			if (err == ERROR_DIR_NOT_EMPTY) {
				setErrorMessage("目标目录非空，无法覆盖：" + dst);
				return FileSystemError::DirectoryNotEmpty;
			}

			setErrorMessage("移动目录失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		return FileSystemError::Success;
	}

	/**
	 * \brief 删除目录
	 *
	 * \param path 要删除的目录路径
	 * \param recursive 是否递归删除非空目录（默认为 false）
	 * \return FileSystemError::Success 删除成功
	 *         FileSystemError::NotFound 目录不存在
	 *         FileSystemError::InvalidPath 无效路径
	 *         FileSystemError::AccessDenied 程序权限不足
	 *         FileSystemError::DirectoryNotEmpty 目录非空且本次操作为不递归删除
	 *         FileSystemError::IsFile 路径为文件，若需要删除文件，请调用 deleteFile 函数
	 *         FileSystemError::Unknown 未知错误
	 */
	FileSystemError WindowsFileSystem::removeDirectory(const std::string& path) {
		clearErrorMessage();

		if (path.empty()) {
			error_message_ = "路径为空";
			return FileSystemError::InvalidPath;
		}

		std::wstring wpath = ChartoWide(path);
		if (wpath.empty() && !path.empty()) {
			error_message_ = "转换路径为 UTF-16 时失败！路径存在非法字符！";
			return FileSystemError::InvalidPath;
		}

		FileAttributesGetter attrGetter(path);
		if (attrGetter.errType_ != FileSystemError::Success) {
			if (!attrGetter.error_message_.empty())
				setErrorMessage(attrGetter.error_message_);
			return attrGetter.errType_;
		}

		// 检查路径是否为目录（期望目录，拿到文件则返回 IsFile）
		if (!(attrGetter.attrData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			setErrorMessage("路径为文件，请使用 deleteFile 删除文件：" + path);
			return FileSystemError::IsFile;
		}

		// 删除空目录
		if (!RemoveDirectoryW(wpath.c_str())) {
			DWORD err = GetLastError();
			if (err == ERROR_DIR_NOT_EMPTY) {
				setErrorMessage("目录非空，无法删除：" + path);
				return FileSystemError::DirectoryNotEmpty;
			}

			if (err == ERROR_ACCESS_DENIED) {
				setErrorMessage("拒绝访问：" + path);
				return FileSystemError::AccessDenied;
			}

			setErrorMessage("删除目录失败，错误码：" + std::to_string(err));
			return FileSystemError::UnKnown;
		}

		return FileSystemError::Success;
	}

}