#include "winapi_mapping.h"

#ifdef _WIN32
#else

#ifdef __cplusplus
	#include <string>
	#include <cstdlib>
#else
	#include <string.h>
	#include <stdlib.h>
#endif

EXTERN_C void* LoadLibraryW(const wchar_t *libname)
{
	size_t len = wcslen(libname);

	char* tmp = (char*)malloc(len + 1);
	wcstombs(tmp, libname, len);
	tmp[len] = '\0';

	void* result = LoadLibraryA(tmp);

	free(tmp);
	return result;
}

EXTERN_C void* LoadLibraryA(const char *filename)
{
	return (dlopen(filename, RTLD_LOCAL | RTLD_NOW));
}

EXTERN_C void *GetProcAddress(void *libhandle, const char *symbolname)
{
	return dlsym(libhandle, symbolname);
}

EXTERN_C void FreeLibrary(void* libhandle)
{
	dlclose(libhandle);
}

EXTERN_C void localtime_s(struct tm* t, const time_t* tim)
{
	localtime_r(tim, t);
}

EXTERN_C void gmtime_s(struct tm* t, const time_t* tim)
{
	gmtime_r(tim, t);
}

EXTERN_C void* _aligned_malloc(size_t n, size_t alignment)
{
	void* mem = NULL;
#if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
	mem = malloc(n);
#else
	posix_memalign(&mem, alignment, n);
#endif

	return mem;
}

EXTERN_C void* _aligned_malloc_dbg(size_t n, size_t alignment, const char* filename, int line)
{
	return _aligned_malloc(n, alignment);
}

EXTERN_C void _aligned_free(void* _Block)
{
	free(_Block);
}

EXTERN_C int wcstombs_s(size_t* converted, char* dst, size_t dstSizeBytes, const wchar_t* src, size_t maxSizeBytes)
{
	return wcstombs(dst, src, maxSizeBytes);
}

EXTERN_C int wcsncpy_s(wchar_t *dest, const wchar_t *src, size_t n)
{
	return wcsncpy(dest, src, n) != NULL;
}

EXTERN_C int closesocket(SOCKET skt)
{
	return close(skt);
}

#endif
