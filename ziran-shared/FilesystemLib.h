#pragma once

// feature check for C++17/TS support state - filesystem is supported on both MSVS2017 and GCC8, but
// on MSVC it's still in experimental namespace, contrary to GCC8, where it's considered stable
// we may also decide to disable C++17 FS support on some platforms (e.g.; some Android targets) with STDCPP_FS_DISABLED macro
#if !defined(STDCPP_FS_DISABLED) && __has_include(<filesystem>)
	#include <filesystem>
	namespace filesystem = std::filesystem;
#else
	#error "Filesystem API (C++17) required"
#endif

#include <string>
#include <list>

#include "hresult.h"

// resolves application directory (may be different from working directory)
filesystem::path Get_Application_Dir();
