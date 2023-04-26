#include "OpCleanUp.h"
#include "CommonFuncs.h"
#include "extern_def.h"

#include "Trace.h"
#include "OpCleanUp.tmh"

FLT_PREOP_CALLBACK_STATUS
OpPreCleanUpOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{

}

FLT_POSTOP_CALLBACK_STATUS
OpPostCleanUpOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{

}