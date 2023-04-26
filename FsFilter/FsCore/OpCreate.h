#ifndef _FSFILTER_OPCREATE_H
#define _FSFILTER_OPCREATE_H

#include "../FsCommon/Common.h"
#include "CtxCallbacks.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

NTSTATUS
FileReparseOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ UNICODE_STRING RedirectPath,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects
);

NTSTATUS
FsFilterSetReparsePath(
	_In_ PFLT_CALLBACK_DATA Data,
	_Inout_ PUNICODE_STRING pRedirectPath,
	_In_ PFSFILTER_INSTANCE_CONTEXT pInstanceCtx
);

#endif // !_FSFILTER_OPCREATE_H
