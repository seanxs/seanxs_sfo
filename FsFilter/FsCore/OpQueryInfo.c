/*++
Module Name:

	OpQueryInfo.c

Abstract:

	处理IRP_MJ_QUERY_INFORMATION请求

Environment:
	Kernel Mode

--*/

#include "OpQueryInfo.h"
#include "CtxCallbacks.h"
#include "Ecp.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreQueryInfo(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_QUERY_INFORMATION的Pre回调函数

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
		//	处理与文件名相关的Query操作

		if (Data->Iopb->Parameters.QueryFileInformation.FileInformationClass == FileNormalizedNameInformation ||
			Data->Iopb->Parameters.QueryFileInformation.FileInformationClass == FileNameInformation)
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

			if (pFileCtx->m_bIsReparsed)
			{
				pFileCtx->m_bWrite = TRUE;
				KdPrint((__FUNCTION__"=> File : %wZ is written\n", FltObjects->FileObject->FileName));
				ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}
			
		}

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
FsFilterPostQueryInfo(
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

	PFILE_NAME_INFORMATION pFileNameInfo = NULL;
	ULONG uBufferSize = 0;
	ULONG uLocalFileNameLen = 0;
	ULONG uFileNameLen = 0;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}
		//	可能需要将重定向后的路径字串进行修正，去掉前缀
		//	Todo
		pFileNameInfo = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
		uBufferSize = Data->Iopb->Parameters.QueryFileInformation.Length;
		uFileNameLen = pFileNameInfo->FileNameLength;
		uLocalFileNameLen = (ULONG)(pFileNameInfo->FileNameLength - 24*2);	//	\\RUIJIEVIRTUALDESKTOPAPD
		pFileNameInfo->FileNameLength = uLocalFileNameLen;
		RtlCopyMemory(pFileNameInfo->FileName, pFileNameInfo->FileName + 24, uLocalFileNameLen);
		Data->IoStatus.Information = uLocalFileNameLen + 2 * sizeof(WCHAR);

		if (!NT_SUCCESS(Data->IoStatus.Status))
		{
			//	有错误发生
			KdPrint((__FUNCTION__"=> Query Infor failed! error code : 0x%X, File Name : %wZ\n",
				Data->IoStatus.Status,
				FltObjects->FileObject->FileName));
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