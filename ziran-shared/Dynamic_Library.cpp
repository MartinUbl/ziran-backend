#include "Dynamic_Library.h"

#include <algorithm>
#include <cwchar>

#ifdef _WIN32
	const wchar_t* rsShared_Object_Extension = L".dll";
#elif __APPLE__
	const wchar_t* rsShared_Object_Extension = L".dylib";
#else
	const wchar_t* rsShared_Object_Extension = L".so";
#endif

CDynamic_Library::CDynamic_Library() noexcept : mHandle(nullptr)
{
	//
}

CDynamic_Library::CDynamic_Library(CDynamic_Library&& other) noexcept : mHandle(nullptr)
{
	std::swap(mHandle, other.mHandle);
	mLib_Path = std::move(other.mLib_Path);
}

CDynamic_Library::~CDynamic_Library() noexcept
{
	if (mHandle)
		Unload();
}

bool CDynamic_Library::Load(const filesystem::path &file_path) noexcept
{
	mLib_Path = file_path;

	const std::wstring converted_path{ mLib_Path.wstring() }; //we need to make a deep copy
	const auto cstr = converted_path.c_str();
    mHandle = LoadLibraryW(cstr);

	// if the library was not found, and is requested by a relative path without leading dot, explicitly try to search in "current" directory;
	// GNU/Linux and macOS does not do so automatically, so when "libname.so" is requested, we need to explicitly try "./libname.so"
	// to find it in current location; the same applies to other relative paths, such as "filters/libname.so" --> "./filters/libname.so"
	if (mHandle == NULL && mLib_Path.is_relative() && !converted_path.empty() && converted_path[0] != '.')
	{
		// construct temporary path instance due to bug in assignment operator of filesystem lib (valgrind reports invalid reads and writes)
		const filesystem::path npath = filesystem::path{ std::wstring{ L"." } } / mLib_Path;
		mLib_Path = npath;
		const std::wstring converted_path2{ mLib_Path.wstring() };
		mHandle = LoadLibraryW(converted_path2.c_str());
	}

	return mHandle != NULL;
}

filesystem::path CDynamic_Library::Lib_Path() const noexcept
{
    return mLib_Path;
}

bool CDynamic_Library::Is_Loaded() const noexcept
{
	return (mHandle != nullptr);
}

void CDynamic_Library::Unload() noexcept
{
	if (mHandle)
	{
		FreeLibrary(mHandle);
		mHandle = NULL;
		mLib_Path.clear();
	}
}

void* CDynamic_Library::Resolve(const char* symbolName) noexcept 
{
	if (!mHandle)
		return nullptr;

	return GetProcAddress(mHandle, symbolName);
}

bool CDynamic_Library::Is_Library(const filesystem::path& path) noexcept
{
	const auto ext = path.extension();

	if (ext.empty())
		return false;

	return ext.wstring() == rsShared_Object_Extension;
}
