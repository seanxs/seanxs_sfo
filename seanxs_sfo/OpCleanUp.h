#pragma once
#ifndef _SEANXS_SFO_OPCLEANUP_H
#define _SEANXS_SFO_OPCLEANUP_H

#include "Common.h"

EXTERN_C_START

FLT_PREOP_CALLBACK_STATUS
OpPreCleanUpOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
OpPostCleanUpOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

EXTERN_C_END

#endif // !_SEANXS_SFO_OPCLEANUP_H

