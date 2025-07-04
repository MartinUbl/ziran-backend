#include "hresult.h"

const char* Describe_Error(const HRESULT error)
{
	switch (error)
	{
		case S_OK: return "No error"; break;
		case S_FALSE: return "Succeeded, but no result"; break;
		case ERROR_FILE_NOT_FOUND: return "File not found"; break;
		case ERROR_READ_FAULT: return "Cannot read from the given source"; break;
		case E_INVALIDARG: return "One or more invalid argument(s)"; break;
		case E_NOTIMPL: return "Not implemented"; break;
		case E_UNEXPECTED: return "Catastrophic failure"; break;
		case E_FAIL: return "Unspecified error"; break;
		case E_NOINTERFACE: return "No such interface"; break;
		case E_ABORT: return "Operation aborted"; break;
		case E_ILLEGAL_METHOD_CALL: return "Method cannot be called at this time"; break;
		case E_ILLEGAL_STATE_CHANGE: return "Attempted illegal state change"; break;
		case ERROR_DS_DRA_EXTN_CONNECTION_FAILED: return "No object to complete the operation"; break;
		case E_ACCESSDENIED: return "Access denied"; break;
		case /*E_NOT_SET*/((HRESULT)0x80070490L): return "Not set/found"; break;	//otherwise fails to compile on Win
		case MK_E_CANTOPENFILE: return "Cannot open file"; break;
		case CO_E_ERRORINDLL: return "Dynamic-libray error"; break;
		default: return "Error description is not available"; break;
	}
}

bool Succeeded(const HRESULT rc)
{
	return (rc == S_OK) || (rc == S_FALSE);
}
