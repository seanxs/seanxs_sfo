/*++
Module Name:

	OpQueryInfo.c

Abstract:

	处理IRP_MJ_QUERY_VOLUME_INFORMATION请求

Environment:
	Kernel Mode

--*/

#include "OpQueryVolume.h"
#include "CtxCallbacks.h"
#include "Ecp.h"
#include "..\FsCommon\WorkItems.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreQueryVolume(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_QUERY_VOLUME_INFORMATION的Pre回调函数

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
	PFSFILTER_INSTANCE_CONTEXT pInstanceCtx = NULL;
	UNICODE_STRING VolumeName = { 0 };
	ULONG BufferSize = 0;

	do
	{
		sts = FltGetInstanceContext(FltObjects->Instance, (PFLT_CONTEXT)&pInstanceCtx);
		if (sts == STATUS_NOT_FOUND)
		{
			//	没有找到context，说明这个volume里面的全部操作，都不需要重定向
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			//break;
		}
		else if (!NT_SUCCESS(sts))
		{
			//	异常错误
			ReturnValue = FLT_PREOP_COMPLETE;
			break;
		}

		//	需要重新计算磁盘空间的volume

		if (Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass == FileFsFullSizeInformation || 
			Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass == FileFsFullSizeInformationEx ||
			Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass == FileFsSizeInformation)
		{
			sts = FltGetVolumeName(FltObjects->Volume, NULL, &BufferSize);
			if (sts == STATUS_BUFFER_TOO_SMALL)
			{
				sts = FsFltAllocUnicodeString((USHORT)BufferSize, PagedPool, &VolumeName);
				if (!NT_SUCCESS(sts))
				{
					break;
				}

				sts = FltGetVolumeName(FltObjects->Volume, &VolumeName, NULL);
				if (!NT_SUCCESS(sts))
				{
					break;
				}

				/*KdPrint((__FUNCTION__"=> Volume : %wZ, FsInformationClass : %d\n",
					VolumeName,
					Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass));*/
			}

			ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

		}
		
		sts = STATUS_SUCCESS;

	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	FsFilterFreeUnicodeString(&VolumeName);

	if (pInstanceCtx)
	{
		FltReleaseContext(pInstanceCtx);
	}

	return ReturnValue;
}


FLT_POSTOP_CALLBACK_STATUS
FsFilterPostQueryVolume(
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
	PFSFILTER_INSTANCE_CONTEXT pInstanceCtx = NULL;
	UNICODE_STRING VolumeName = { 0 };
	ULONG BufferSize = 0;

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

		sts = FltGetInstanceContext(FltObjects->Instance, (PFLT_CONTEXT)&pInstanceCtx);
		if (sts == STATUS_NOT_FOUND)
		{
			//	没有找到context，说明这个volume里面的全部操作，都不需要重定向
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			//break;
		}
		else if (!NT_SUCCESS(sts))
		{
			//	异常错误
			ReturnValue = FLT_PREOP_COMPLETE;
			break;
		}

		//	需要重新计算磁盘空间的volume

		if (Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass == FileFsFullSizeInformation ||
			Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass == FileFsFullSizeInformationEx ||
			Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass == FileFsSizeInformation)
		{
			sts = FltGetVolumeName(FltObjects->Volume, NULL, &BufferSize);
			if (sts == STATUS_BUFFER_TOO_SMALL)
			{
				sts = FsFltAllocUnicodeString((USHORT)BufferSize, PagedPool, &VolumeName);
				if (!NT_SUCCESS(sts))
				{
					break;
				}

				sts = FltGetVolumeName(FltObjects->Volume, &VolumeName, NULL);
				if (!NT_SUCCESS(sts))
				{
					break;
				}

				/*KdPrint((__FUNCTION__"=> Volume : %wZ, FsInformationClass : %d\n",
					VolumeName,
					Data->Iopb->Parameters.QueryVolumeInformation.FsInformationClass));*/
			}

			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
		}

		sts = STATUS_SUCCESS;

	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	FsFilterFreeUnicodeString(&VolumeName);

	if (pInstanceCtx)
	{
		FltReleaseContext(pInstanceCtx);
	}

	return ReturnValue;
}