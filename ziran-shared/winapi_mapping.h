#ifndef INC_WINAPI_MAPPING_H
#define INC_WINAPI_MAPPING_H

#ifndef EXTERN_C
	#ifdef __cplusplus
		#define EXTERN_C extern "C"
	#else
		#define EXTERN_C extern
	#endif
#endif

#ifdef _WIN32
	#include <Windows.h>
	#include <locale.h>

	EXTERN_C char __ImageBase;

	typedef int socklen_t;

	#define MainCalling __cdecl

	#define WSTRING_FORMATTER "%ws"

	inline _locale_t getlocale(int cat) {
		return _get_current_locale();
	}
#else
	#include <unistd.h>
	#include <dlfcn.h>
	#include <errno.h>
	#include <sys/ioctl.h>

#ifdef __cplusplus
	#include <ctime>
	#include <clocale>
#else
	#include <time.h>
	#include <wchar.h>
	#include <locale.h>
#endif

	#define MainCalling
	
	typedef int BOOL;
	#define TRUE ((BOOL)1)
#ifndef FALSE
	// strangely, on some systems, FALSE constant is not defined
	// NOTE: this has to be #define, as some libraries may check for this constant using preprocessor macro
	#define FALSE ((BOOL)0)
#endif

	typedef void* HMODULE;

	EXTERN_C void localtime_s(struct tm* t, const time_t* tim);
	EXTERN_C void gmtime_s(struct tm* t, const time_t* tim);
	EXTERN_C void _get_timezone(long* tzdst);

	EXTERN_C void* LoadLibraryW(const wchar_t *filename);
	EXTERN_C void* LoadLibraryA(const char *filename);
	EXTERN_C void* GetProcAddress(void *libhandle, const char *symbolname);
	EXTERN_C void FreeLibrary(void* libhandle);

	EXTERN_C int closesocket(int fd);

	#define SOCKET int
	#define InetPtonA inet_pton
	#define InetPton InetPtonA
	#define InetNtopA inet_ntop
	#define InetNtop InetNtopA
	#define WSAEINPROGRESS EINPROGRESS
	#define INVALID_SOCKET (SOCKET)(~0)
	#define wcstok_s wcstok

	static inline int WSAGetLastError() {
		return errno;
	}

	static inline int ioctlsocket(int fd, unsigned long request, void* arg) {
		return ioctl(fd, request, arg);
	}

	static inline char* getlocale(int cat) {
		return setlocale(cat, NULL);
	}

	#define swscanf_s swscanf

	EXTERN_C void* _aligned_malloc(size_t n, size_t alignment);
	EXTERN_C void* _aligned_malloc_dbg(size_t n, size_t alignment, const char* filename, int line);
	EXTERN_C void _aligned_free(void* _Block);

	EXTERN_C int wcstombs_s(size_t* converted, char* dst, size_t dstSizeBytes, const wchar_t* src, size_t maxSizeBytes);
	EXTERN_C int wcsncpy_s(wchar_t *dest, const wchar_t *src, size_t n);

	#define SD_BOTH SHUT_RDWR

	#define WSTRING_FORMATTER "%ls"
#endif

#endif
