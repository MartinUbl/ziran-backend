#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
	#include <WTypes.h>
#else
	typedef long HRESULT;

	typedef unsigned long	DWORD;
	typedef long			LONG;
	typedef unsigned long	ULONG;
	// define macro to avoid redefinition in several used libraries (e.g. ExcelFormat)
	#define INT32_TYPES_DEFINED

	#define S_OK									((HRESULT)0L)
	#define S_FALSE									((HRESULT)1L)
	#define ERROR_FILE_NOT_FOUND					((HRESULT)2L)
	#define ERROR_READ_FAULT						((HRESULT)30L)
	#define ERROR_DS_DRA_EXTN_CONNECTION_FAILED		((HRESULT)8466L)

	#define E_NOTIMPL								((HRESULT)0x80004001L)
	#define E_UNEXPECTED							((HRESULT)0x8000FFFFL)
	#define E_FAIL									((HRESULT)0x80004005L)
	#define E_NOINTERFACE							((HRESULT)0x80004002L)
	#define E_ABORT									((HRESULT)0x80004004L)
	#define E_ILLEGAL_METHOD_CALL					((HRESULT)0x8000000EL)
	#define E_ILLEGAL_STATE_CHANGE					((HRESULT)0x8000000DL)
	#define E_ACCESSDENIED							((HRESULT)0x80070005L)
	#define TYPE_E_AMBIGUOUSNAME					((HRESULT)0x8002802CL)
	#define E_INVALIDARG							((HRESULT)0x80070057L)
	#define MK_E_CANTOPENFILE						((HRESULT)0x800401EAL)
	#define MK_E_UNAVAILABLE						((HRESULT)0x800401E3L)
	#define CO_E_ERRORINDLL							((HRESULT)0x800401F9L)
	#define E_HANDLE								((HRESULT)0x80070006L)
	#define E_OUTOFMEMORY							((HRESULT)0x8007000EL)
	#define E_NOT_SET								((HRESULT)0x80070490L)
#endif

#ifdef __cplusplus
	extern "C"
#endif
const char* Describe_Error(const HRESULT error);

#ifdef __cplusplus
	extern "C"
#endif
bool Succeeded(const HRESULT rc);
