#pragma once
#ifndef _APPL_FSFILTER_OPCLOSE_H
#define _APPL_FSFILTER_OPCLOSE_H

#include "..\FsCommon\Common.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreClose(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostClose(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

#endif // !_APPL_FSFILTER_OPCLOSE_H