/*++
Module Name:

	OpWrite.c

Abstract:

	处理IRP_MJ_WRITE请求
	基于minifilter技术

Environment:
	Kernel Mode

--*/

#include "OpWrite.h"
#include "CtxCallbacks.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_WRITE的Pre回调函数

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
		sts = FltGetFileContext(
			FltObjects->Instance,
			FltObjects->FileObject,
			&pFileCtx);
		if (!NT_SUCCESS(sts))
		{
			if (sts == STATUS_NOT_FOUND ||
				sts == STATUS_NOT_SUPPORTED)
			{
				//	没有file context，说明没有reparse
				sts = STATUS_SUCCESS;
				ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
				break;

			}
			else
			{
				//	有错误发生
				break;
			}

		}

		if (pFileCtx->m_bIsReparseBack)
		{
			pFileCtx->m_bWrite = TRUE;
			KdPrint((__FUNCTION__"=> File : %wZ is written\n", FltObjects->FileObject->FileName));
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
FsFilterPostWrite(
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
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	do
	{
		if (!FltSupportsFileContexts(FltObjects->FileObject))
		{
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			break;
		}

		sts = FltGetFileContext(
			FltObjects->Instance,
			FltObjects->FileObject,
			&pFileCtx);
		if (!NT_SUCCESS(sts))
		{
			if (sts == STATUS_NOT_FOUND)
			{
				//	没有file context，说明没有reparse
				sts = STATUS_SUCCESS;
				ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
				break;

			}
			else
			{
				//	有错误发生
				break;
			}

		}
		
		KdPrint((__FUNCTION__"=> File : %wZ IoStatus.Status : %d\n",
			FltObjects->FileObject->FileName,
			Data->IoStatus.Status));

		sts = STATUS_SUCCESS;
	} while (0);

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	return ReturnValue;
}