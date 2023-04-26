/*++
Module Name:

	OpQueryInfo.c

Abstract:

	处理IRP_MJ_SET_SECURITY请求

Environment:
	Kernel Mode

--*/

#include "OpSetSecurity.h"
#include "CtxCallbacks.h"
#include "Ecp.h"
#include "..\FsCommon\WorkItems.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreSetSecurity(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_SET_SECURITY的Pre回调函数

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

		//	经过重定向的文件，设置FLT_PREOP_SUCCESS_WITH_CALLBACK
		KdPrint((__FUNCTION__"=> Set Security on File : %wZ\n", FltObjects->FileObject->FileName));
		ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
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
FsFilterPostSetSecurity(
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
	PFLT_DEFERRED_IO_WORKITEM pFltDeferWorkItem = NULL;
	PFSFILTER_WORKITEMCTX_SETSECURITY pWorkItemCtxSetSecurity = NULL;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		//	依据文件系统处理结果，判断是否要对源文件执行同样操作
		//	Todo
		if (!NT_SUCCESS(Data->IoStatus.Status))
		{
			//	有错误发生
			KdPrint((__FUNCTION__"=> Set Security failed! error code : %0xX, File Name : %wZ\n",
				Data->IoStatus.Status,
				FltObjects->FileObject->FileName));
			break;
		}

		sts = ApplFltGetFileCtx(FltObjects->Instance, FltObjects->FileObject, &pFileCtx);
		if (!NT_SUCCESS(sts))
		{
			if (sts == STATUS_NOT_FOUND ||
				sts == STATUS_NOT_SUPPORTED)
			{
				//	没有reparse
				sts = STATUS_SUCCESS;
				ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
				break;
			}
			else
			{
				//	有错误发生
				ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
				break;
			}

		}

		//	将操作插入workitem
		pFltDeferWorkItem = FltAllocateDeferredIoWorkItem();
		if (pFltDeferWorkItem == NULL)
		{
			//	出错了
			sts = STATUS_UNSUCCESSFUL;
			break;
		}

		pWorkItemCtxSetSecurity = ExAllocatePoolWithTag(PagedPool, sizeof(FSFILTER_WORKITEMCTX_SETSECURITY), 'twic');
		if (pWorkItemCtxSetSecurity == NULL)
		{
			sts = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		//	需要添加实际使用的参数，此处示例而已
		pWorkItemCtxSetSecurity->PreOrPost = WORKITEM_PREORPOST_POSTCALLBACK;
		sts = FltQueueDeferredIoWorkItem(pFltDeferWorkItem, Data, WorkItem_SetSecurity, DelayedWorkQueue, pWorkItemCtxSetSecurity);
		if (!NT_SUCCESS(sts))
		{
			//	出错了
			break;
		}
		else
		{
			ReturnValue = FLT_POSTOP_MORE_PROCESSING_REQUIRED;
		}

		sts = STATUS_SUCCESS;

	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));

		if (pWorkItemCtxSetSecurity)
		{
			ExFreePoolWithTag(pWorkItemCtxSetSecurity, 'twic');
		}

		if (pFltDeferWorkItem)
		{
			FltFreeDeferredIoWorkItem(pFltDeferWorkItem);
		}
	}

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	return ReturnValue;
}