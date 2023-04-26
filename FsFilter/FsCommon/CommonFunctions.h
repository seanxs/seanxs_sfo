#ifndef _APPL_FSFILTER_COMMONFUNCTION
#define _APPL_FSFILTER_COMMONFUNCTION

#include "Common.h"
#include "..\FsCore\Ecp.h"

#define UNISTR_TAG 'pstr'

EXTERN_C_START

NTSTATUS FsFltAllocUnicodeString(
	_In_ USHORT Size,
	_In_ POOL_TYPE pool,
	_Inout_ PUNICODE_STRING pString
);

NTSTATUS FsFltCopyUnicodeString(
	_In_ PUNICODE_STRING pDstUniStr,
	_In_ PUNICODE_STRING pSrcUniStr
);

NTSTATUS FsFilterFreeUnicodeString(
	_Inout_ PUNICODE_STRING pString
);

NTSTATUS ApplFsOpenFile(
	_In_ PUNICODE_STRING pFileName,
	_In_ PFLT_INSTANCE pInstance,
	_Inout_ PHANDLE phFile,
	_Inout_ PFILE_OBJECT *pFileObj
);

NTSTATUS ApplFsGetFileSize(
	_In_ PFILE_OBJECT pFileObj,
	_In_ PFLT_INSTANCE pInstance,
	_Inout_ LARGE_INTEGER *pSize
);

NTSTATUS FsFltReadFile(
	_In_ PFILE_OBJECT pFileObj,
	_In_ PFLT_INSTANCE pInstance,
	_In_ PVOID pBuf,
	_In_ ULONG Size,
	_In_ PLARGE_INTEGER Offset,
	_Inout_ ULONG_PTR *pReadLen
);

NTSTATUS GetParsedNameInfo(
	_In_ PFLT_CALLBACK_DATA pCbd,
	_Inout_ PFLT_FILE_NAME_INFORMATION *pNameInfo);


NTSTATUS FsFltFileOrFolder(
	_In_ PUNICODE_STRING pFileName,
	_In_ PFLT_INSTANCE pInstance,
	_Inout_ PBOOLEAN phFile
);
NTSTATUS
VDFileFilterAllocateUnicodeString(
	_Inout_ PUNICODE_STRING String,
	_In_	ULONG	uTag
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
	ULONG    SystemInformationClass,
	PVOID    SystemInformation,
	ULONG    SystemInformationLength,
	PULONG    ReturnLength
);

VOID
VDFileFilterFreeUnicodeString(
	_Inout_ PUNICODE_STRING String,
	_In_	ULONG	uTag
);

EXTERN_C_END

#endif // !_APPL_FSFILTER_COMMONFUNCTION
