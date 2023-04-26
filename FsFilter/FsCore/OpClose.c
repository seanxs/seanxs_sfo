/*++
Module Name:

	OpClose.c

Abstract:

	处理IRP_MJ_CLOSE请求的模块

Environment:
	Kernel Mode

--*/

#include "OpClose.h"
#include "CtxCallbacks.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreClose(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_CLOSE的Pre回调函数

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
	PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	do
	{
		sts = ApplFltGetFileCtx(FltObjects->Instance, FltObjects->FileObject, &pFileCtx);
		if (!NT_SUCCESS(sts))
		{
			if (sts == STATUS_NOT_FOUND ||
				sts == STATUS_NOT_SUPPORTED)
			{
				//	没有reparse
				sts = STATUS_SUCCESS;
				ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
				break;
			}
			else
			{
				//	有错误发生
				ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
				break;
			}
		}

		if (pFileCtx->m_bWrite)
		{
			KdPrint((__FUNCTION__"=> File : %wZ will copy to remote\n",
				FltObjects->FileObject->FileName));
			ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
		}

		if (pFileCtx->m_bDelete)
		{
			KdPrint((__FUNCTION__"=> File : %wZ is deleted, File : %wZ will also be deleted\n",
				FltObjects->FileObject->FileName,
				pFileCtx->OriginalFileName));
			ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
		}

		sts = STATUS_SUCCESS;
	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
		ReturnValue = FLT_PREOP_COMPLETE;
	}

	if (pNameInfo)
	{
		FltReleaseFileNameInformation(pNameInfo);
	}

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostClose(
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
	NTSTATUS sts = STATUS_UNSUCCESSFUL;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		sts = STATUS_SUCCESS;
	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	return ReturnValue;
}