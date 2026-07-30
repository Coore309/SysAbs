#include <sysabs/ifilesystem.h>
#include <sysabs/platform/windows_filesystem.h>

namespace sysabs {
	std::unique_ptr<IFileSystem> createFileSystem() {
#ifdef _WIN32
		return std::make_unique<WindowsFileSystem>();
#elif defined(__ANDROID__)
		return nullptr;
#elif defined(__linux__) && !defined(__ANDROID__)
		return nullptr;
#endif
	}
}