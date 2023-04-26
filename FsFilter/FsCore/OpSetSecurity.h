#ifndef _FSFILTER_OPSETSETURITY_H
#define _FSFILTER_OPSETSETURITY_H

#include "..\FsCommon\Common.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreSetSecurity(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostSetSecurity(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

#endif // !_FSFILTER_OPSETSETURITY_H
