#pragma once
#ifndef _SEANXS_SFO_OPSETINFO_H
#define _SEANXS_SFO_OPSETINFO_H

#include "Common.h"

EXTERN_C_START

FLT_PREOP_CALLBACK_STATUS
OpPreSetInfoOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
OpPostSetInfoOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

EXTERN_C_END

#endif // _SEANXS_SFO_OPSETINFO_H