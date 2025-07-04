
#ifdef __APPLE__
	// small hack to avoid redefinition of TRUE and FALSE in non-strongly typed manner
	// this "converts" Apple definition of DYLD_BOOL to strongly typed enum, therefore
	// allowing us to define our own "global" TRUE and FALSE
	#define DYLD_BOOL class DYLD_BOOL

	#include <mach-o/dyld.h>
	#include <dlfcn.h>
#endif

#include "FilesystemLib.h"

#include "hresult.h"
#include "winapi_mapping.h"

#include <cstring>
#include <algorithm>
#include <cwctype>

// PATH_MAX is fixed on several platforms (e.g. Android) to a fairly big number (4096). We don't need so long paths on a regular system,
// so we define it to be lower. However, Android-NDK verifies the length of an input buffer to be >= PATH_MAX, so we have to be consistent
#ifdef PATH_MAX
	const size_t Max_File_Path = PATH_MAX;
#else
	//On Windows, there's MAX_PATH==260, which is too little => hence we do not check for its presence
	const size_t Max_File_Path = 1024;
#endif


filesystem::path Get_Application_Dir()
{
#ifdef _WIN32
	wchar_t ModuleFileName[Max_File_Path];
	GetModuleFileNameW(NULL, ModuleFileName, Max_File_Path);
#elif __APPLE__
	char RelModuleFileName[Max_File_Path];
	uint32_t size = static_cast<uint32_t>(Max_File_Path);
	_NSGetExecutablePath(RelModuleFileName, &size);

	char ModuleFileName[Max_File_Path];
	realpath(RelModuleFileName, ModuleFileName);
#else
	char ModuleFileName[Max_File_Path];
	memset(ModuleFileName, 0, Max_File_Path);
	readlink("/proc/self/exe", ModuleFileName, Max_File_Path);
	// TODO: error checking
#endif

	filesystem::path exe_path{ ModuleFileName };

	return exe_path.remove_filename();
}
