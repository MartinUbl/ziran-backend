#pragma once

#include <string>
#include <memory>

#include "FilesystemLib.h"
#include "winapi_mapping.h"

/*
 * Dynamic library (shared object) wrapper class
 */
class CDynamic_Library final
{
	private:
		// stored module handle (nullptr if invalid)
		HMODULE mHandle;
		// loaded library path
		filesystem::path mLib_Path;

	public:
		CDynamic_Library() noexcept;
		// disallow copying - the handle has to be unique
		CDynamic_Library(const CDynamic_Library&) = delete;
		CDynamic_Library(CDynamic_Library&& other) noexcept;
		virtual ~CDynamic_Library() noexcept;

		// loads module and returns result
		bool Load(const filesystem::path &file_path) noexcept;
		// unloads module if loaded
		void Unload() noexcept;
		// resolves symbol from loaded module; returns nullptr if no such symbol found or no module loaded
		void* Resolve(const char* symbolName) noexcept;

		template<typename T>
		T Resolve(const char* symbolName) noexcept {
			return reinterpret_cast<T>(Resolve(symbolName));
		}

		// is module (properly) loaded?
		bool Is_Loaded() const noexcept;
		filesystem::path Lib_Path() const noexcept;

		// checks extension of supplied path to verify, if it's a library (platform-dependent check)
		static bool Is_Library(const filesystem::path& path) noexcept;
};
