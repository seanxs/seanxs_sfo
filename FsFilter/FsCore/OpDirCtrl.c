/*++
Module Name:

	OpDirCtrl.c

Abstract:

	处理IRP_MJ_DIRECTORY_CONTROL请求的模块
	查询，刷新，合并&去重

Environment:
	Kernel Mode

--*/

#include "OpDirCtrl.h"
#include "CtxCallbacks.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreDirCtrl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_DIRECTORY_CONTROL的Pre回调函数

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
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFSFILTER_INSTANCE_CONTEXT pInstanceCtx = NULL;
	PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	do
	{
		//	测试代码

		sts = FltGetInstanceContext(FltObjects->Instance, (PFLT_CONTEXT)&pInstanceCtx);
		if (sts == STATUS_NOT_FOUND)
		{
			//	没有找到context，说明这个volume里面全部操作，都不需要重定向
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			break;
		}
		else if (!NT_SUCCESS(sts))
		{
			//	异常错误
			ReturnValue = FLT_PREOP_COMPLETE;
			break;
		}

		//	获取FILE_CONTEXT参数
		//if (FltObjects->FileObject)
		//{
		//	if (!FltSupportsFileContexts(FltObjects->FileObject))
		//	{
		//		sts = STATUS_SUCCESS;
		//		ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
		//		break;
		//	}
		//	sts = FltGetFileContext(
		//		FltObjects->Instance,
		//		FltObjects->FileObject,
		//		&pFileCtx);
		//	if (!NT_SUCCESS(sts))
		//	{
		//		if (sts == STATUS_NOT_FOUND)
		//		{
		//			//	没有file context，说明没有reparse
		//			sts = STATUS_SUCCESS;
		//			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
		//			break;
		//			
		//		}
		//		else
		//		{
		//			//	有错误发生
		//			break;
		//		}
		//		
		//	}
		//}

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

		if(pFileCtx && pFileCtx->m_bIsReparsed)
		{
			//	需要进行合并&去重操作
			sts = GetParsedNameInfo(Data, &pNameInfo);
			if (!NT_SUCCESS(sts))
			{
				//	有错误发生
				break;
			}
			
			KdPrint((__FUNCTION__"=> Instance : %p, Path : %wZ, MinorFunction : %d, OperationFlags : %d\n",
				FltObjects->Instance,
				&pNameInfo->Name,
				Data->Iopb->MinorFunction,
				Data->Iopb->OperationFlags));

			if (Data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY)
			{
				KdPrint((__FUNCTION__"=> QueryDirectory.FileInformationClass : %d, ReturnSingleEntry : %d, bRestartScan : %d\n",
					Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass,
					BooleanFlagOn(Data->Iopb->OperationFlags, SL_RETURN_SINGLE_ENTRY),
					BooleanFlagOn(Data->Iopb->OperationFlags, SL_RESTART_SCAN)));
				ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}
			/*else if (Data->Iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY)
			{
				KdPrint((__FUNCTION__"=> NotifyDirectory\n"));
			}
			else if (Data->Iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY_EX)
			{
				KdPrint((__FUNCTION__"=> NotifyDirectoryEx.DirectoryNotifyInformationClass : %d\n",
					Data->Iopb->Parameters.DirectoryControl.NotifyDirectoryEx.DirectoryNotifyInformationClass));
			}*/

			////
			//KdPrint((__FUNCTION__"=> Original Instance : %p, Original Path : %wZ\n",
			//	pFileCtx->pOriginalInstance,
			//	&pFileCtx->OriginalFileName));
		}
		else
		{
			//	不需要进行合并&去重操作
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
		}

		//	测试代码

	} while (0);

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	if (pNameInfo)
	{
		FltReleaseFileNameInformation(pNameInfo);
	}

	if (pInstanceCtx)
	{
		FltReleaseContext(pInstanceCtx);
	}

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
		ReturnValue = FLT_PREOP_COMPLETE;
	}

	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostDirCtrl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

	IRP_MJ_DIRECTORY_CONTROL的Post回调函数

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
	PFSFILTER_INSTANCE_CONTEXT pInstanceCtx = NULL;
	//PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		//	测试代码
		sts = FltGetInstanceContext(FltObjects->Instance, (PFLT_CONTEXT)&pInstanceCtx);
		if (sts == STATUS_NOT_FOUND)
		{
			//	没有找到context，说明这个volume里面全部操作，都不需要重定向
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}
		else if (!NT_SUCCESS(sts))
		{
			//	异常错误
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		//	获取FILE_CONTEXT参数
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
				ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
				break;

			}
			else
			{
				//	有错误发生
				break;
			}

		}

		if (NT_SUCCESS(Data->IoStatus.Status))
		{
			if (Data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY)
			{
				KdPrint((__FUNCTION__"=> Directory : %wZ enum succeed, FileInformationClass : %d, returned size : %d\n",
					FltObjects->FileObject->FileName,
					Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass,
					Data->IoStatus.Information));
				KdPrint((__FUNCTION__"=> need to enum original directory : %wZ\n", pFileCtx->OriginalFileName));
				if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation)
				{
					KdPrint((__FUNCTION__"=> entry count : %d\n", Data->IoStatus.Information / sizeof(FILE_ID_BOTH_DIR_INFORMATION)));
				}
			}
		}
		//	测试代码

	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	if (pInstanceCtx)
	{
		FltReleaseContext(pInstanceCtx);
	}

	return ReturnValue;
}