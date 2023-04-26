/*++
Module Name:

	OpSetInformation.c

Abstract:

	处理IRP_MJ_SET_INFORMATION请求
	基于minifilter技术

Environment:
	Kernel Mode

--*/

#include "OpCreate.h"
#include "CtxCallbacks.h"
#include "..\FsCommon\CommonFunctions.h"

#include "..\FsCommon\Trace.h"
#include "OpSetInformation.tmh"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreSetInformation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_SET_INFORMATION的Pre回调函数

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The context for the completion routine for this
		operation.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	//PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	if (FLT_IS_FASTIO_OPERATION(Data))
	{
		return FLT_PREOP_DISALLOW_FASTIO;
	}

	//KdPrint((__FUNCTION__"=> File Name : %wZ\n", FltObjects->FileObject->FileName));

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

		switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass)
		{
		case FileRenameInformation:
		case FileRenameInformationEx:
		{
			//	设置重命名标记
			PFILE_RENAME_INFORMATION pRenameInfo = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			/*KdPrint((__FUNCTION__"=> FileRenameInformation, old file name : %wZ, new file name : %ws\n",
				pFileCtx->OriginalFileName,
				pRenameInfo->FileName));*/
			DoTraceLevelMessage(
				TRACE_LEVEL_INFORMATION,
				SEANXS_SFO_SETINFO,
				"%!FILE!,%!FUNC!,%!LINE!=> FileRenameInformation,FileObject : 0x%p, new file name : %ws\n",
				FltObjects->FileObject,
				pRenameInfo->FileName);
			pFileCtx->m_bRename = TRUE;
			*CompletionContext = pFileCtx;
			ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			break;
		}
		case FileDispositionInformation:
		case FileDispositionInformationEx:
		{
			//	设置删除标记
			KdPrint((__FUNCTION__"=> FileDispositionInformation(Ex), file name : %wZ\n",
				FltObjects->FileObject->FileName));

			pFileCtx->m_bDelete = TRUE;
			*CompletionContext = pFileCtx;
			ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			break;
		}
		default:
			break;
		}
	} while (0);
	
	if (pFileCtx && ReturnValue != FLT_PREOP_SUCCESS_WITH_CALLBACK)
	{
		FltReleaseContext(pFileCtx);
	}

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}
	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostSetInformation(
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
	PFSFILTER_FILE_CONTEXT pFileCtx = CompletionContext;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		if (!NT_SUCCESS(Data->IoStatus.Status))
		{
			//	有错误发生
			/*KdPrint((__FUNCTION__"=> Set Information failed! error code : 0x%X, File Name : %wZ\n",
				Data->IoStatus.Status,
				FltObjects->FileObject->FileName));*/
			DoTraceLevelMessage(
				TRACE_LEVEL_INFORMATION,
				SEANXS_SFO_SETINFO,
				"%!FILE!,%!FUNC!,%!LINE!=> Failed! FileInformationClass : %d, FileObject : 0x%p, error code : 0x%X\n",
				Data->Iopb->Parameters.SetFileInformation.FileInformationClass,
				FltObjects->FileObject,
				Data->IoStatus.Status);
			break;
		}

		switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass)
		{
		case FileRenameInformation:
		case FileRenameInformationEx:
		{
			//	设置重命名标记
			//PFILE_RENAME_INFORMATION pRenameInfo = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			/*KdPrint((__FUNCTION__"=> FileRenameInformation, old file name : %wZ, new file name : %ws\n",
				&pFileCtx->OriginalFileName,
				pRenameInfo->FileName));*/
			DoTraceLevelMessage(
				TRACE_LEVEL_INFORMATION,
				SEANXS_SFO_SETINFO,
				"%!FILE!,%!FUNC!,%!LINE!=> FileRenameInformation, FileObject : 0x%p, succeed!\n",
				FltObjects->FileObject);
			pFileCtx->m_bRename = TRUE;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}
		case FileDispositionInformation:
		case FileDispositionInformationEx:
		{
			//	设置删除标记
			KdPrint((__FUNCTION__"=> FileDispositionInformation(Ex), file name : %wZ\n",
				FltObjects->FileObject->FileName));

			pFileCtx->m_bDelete = TRUE;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}
		default:
			break;
		}
		
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