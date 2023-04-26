#pragma once
#ifndef _FSFILTER_NAMEPROVIDER_H
#define _FSFILTER_NAMEPROVIDER_H

#include ".\FsCommon\Common.h"

NTSTATUS FsFilterGeneFileName(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_When_(FileObject->FsContext != NULL, _In_opt_)
	_When_(FileObject->FsContext == NULL, _In_)
	PFLT_CALLBACK_DATA CallbackData,
	_In_ FLT_FILE_NAME_OPTIONS NameOptions,
	_Out_ PBOOLEAN CacheFileNameInformation,
	_Inout_ PFLT_NAME_CONTROL FileName
);

NTSTATUS
FsFilterNormalizeNameComponent(
	_In_ PFLT_INSTANCE Instance,
	_In_ PCUNICODE_STRING ParentDirectory,
	_In_ USHORT DeviceNameLength,
	_In_ PCUNICODE_STRING Component,
	_Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
	_In_ ULONG ExpandComponentNameLength,
	_In_ FLT_NORMALIZE_NAME_FLAGS Flags,
	_Inout_ PVOID *NormalizationContext
);
#endif // !_FSFILTER_NAMEPROVIDER_H
