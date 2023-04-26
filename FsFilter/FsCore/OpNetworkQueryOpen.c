/*++
Module Name:

	OpNetworkQueryOpen.c

Abstract:

	处理IRP_MJ_NETWORK_QUERY_OPEN请求

Environment:
	Kernel Mode

--*/

#include "OpNetworkQueryOpen.h"
#include "CtxCallbacks.h"
#include "Ecp.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreNetworkQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_NETWORK_QUERY_OPEN的Pre回调函数

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The context for the completion routine for this
		operation.

Return Value:

	The return value is the status of the operation.

Logic:


--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	do
	{
		//ReturnValue = FLT_PREOP_DISALLOW_FASTIO;
		ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
		sts = STATUS_SUCCESS;

	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
		ReturnValue = FLT_PREOP_COMPLETE;
	}

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostNetworkQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

	This routine is the post-operation completion routine for this
	miniFilter.

	This is non-pageable because it may be called at DPC level.

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The completion context set in the pre-operation routine.

	Flags - Denotes whether the completion is successful or is being drained.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	FLT_POSTOP_CALLBACK_STATUS ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		sts = STATUS_SUCCESS;
	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%X\n", sts));
	}

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	return ReturnValue;
}