#pragma once
#ifndef _SEANXS_SFO_QUERYINFO_H
#define _SEANXS_SFO_QUERYINFO_H

#include "Common.h"

EXTERN_C_START

FLT_PREOP_CALLBACK_STATUS
OpPreQueryInfoOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
OpPostQueryInfoOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

EXTERN_C_END

#endif // _SEANXS_SFO_QUERYINFO_H

