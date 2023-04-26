#pragma once
#ifndef _SEANXS_SFO_COMMONFUNCS_H
#define _SEANXS_SFO_COMMONFUNCS_H

#include "Common.h"

#define UNISTR_TAG 'ustr'

NTSTATUS FsFilterFreeUnicodeString(
	_Inout_ PUNICODE_STRING pString
);

NTSTATUS FsFltAllocUnicodeString(
	_In_ USHORT Size,
	_In_ POOL_TYPE pool,
	_Inout_ PUNICODE_STRING pString
);

NTSTATUS GetParsedNameInfo(
	_In_ PFLT_CALLBACK_DATA pCbd,
	_Inout_ PFLT_FILE_NAME_INFORMATION *pNameInfo
);

#endif // !_SEANXS_SFO_COMMONFUNCS_H
