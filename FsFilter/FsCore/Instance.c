/*++
Module Name:

	OpCreate.c

Abstract:

	处理Instance的挂载与卸载等请求，每个volume(卷)都需要有对应的Instance
	这样minifilter驱动才能收到属于这个volume的文件请求

Environment:
	Kernel Mode

--*/

#include "Instance.h"
#include "..\FsCommon\CommonFunctions.h"
#include "CtxCallbacks.h"

#include "..\FsCommon\Trace.h"
#include "Instance.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsFilterInstanceQueryTeardown)
#pragma alloc_text(PAGE, FsFilterInstanceSetup)
#pragma alloc_text(PAGE, FsFilterInstanceTeardownStart)
#pragma alloc_text(PAGE, FsFilterInstanceTeardownComplete)
#endif


NTSTATUS
FsFilterInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/*++

Routine Description:

	This routine is called whenever a new instance is created on a volume. This
	gives us a chance to decide if we need to attach to this volume or not.

	If this routine is not defined in the registration structure, automatic
	instances are always created.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Flags describing the reason for this attach request.

Return Value:

	STATUS_SUCCESS - attach
	STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(Flags);

	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	NTSTATUS rtn = STATUS_FLT_DO_NOT_ATTACH;
	ULONG BufferSize = 0;
	UNICODE_STRING VolumeName = { 0 };
	PFSFILTER_INSTANCE_CONTEXT pInstanceCtx = NULL;

	PAGED_CODE();

	DoTraceLevelMessage(
		TRACE_LEVEL_VERBOSE,
		SEANXS_SFO_INSTANCE,
		"%!FILE!,%!FUNC!,%!LINE!=> enter\n");

	//	获取Volume名
	do
	{
		//	下列文件系统类型的分区不挂载
		if (FLT_FSTYPE_MUP == VolumeFilesystemType ||
			FLT_FSTYPE_RAW == VolumeFilesystemType ||
			FLT_FSTYPE_CDFS == VolumeFilesystemType ||
			FLT_FSTYPE_UDFS == VolumeFilesystemType)
		{
			rtn = STATUS_FLT_DO_NOT_ATTACH;
			break;
		}

		if (FlagOn(Flags, FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT))
		{
			rtn = STATUS_FLT_DO_NOT_ATTACH;
			break;
		}

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

			//KdPrint((__FUNCTION__"=> Volume Name : %wZ attached!\n", VolumeName));
			DoTraceLevelMessage(
				TRACE_LEVEL_VERBOSE,
				SEANXS_SFO_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE!=> Volume Name : %wZ\n, Flags : %X", &VolumeName, Flags);

			//	需要attach这个volume，为Instance创建context
			sts = FsFilterInstanceCtxCreate(FltObjects->Instance, &pInstanceCtx);
			if (!NT_SUCCESS(sts))
			{
				break;
			}

			// 测试代码
			if (FlagOn(Flags, FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT))
			{
				UNICODE_STRING destpath = { 0 };

				sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &pInstanceCtx->DestVolume);
				if (!NT_SUCCESS(sts))
				{
					break;
				}
				RtlInitUnicodeString(&destpath, L"\\Device\\HarddiskVolume8\0");
				RtlAppendUnicodeStringToString(&pInstanceCtx->DestVolume, &destpath);

				sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &pInstanceCtx->DestPrefix);
				if (!NT_SUCCESS(sts))
				{
					break;
				}
				//RtlInitUnicodeString(&destpath, L"\\RUIJIEVIRTUALDESKTOPAPD\0");
				RtlInitUnicodeString(&destpath, L"\\SeanXS_Disk\0");
				RtlAppendUnicodeStringToString(&pInstanceCtx->DestPrefix, &destpath);

				sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &pInstanceCtx->SrcDirPath);
				if (!NT_SUCCESS(sts))
				{
					break;
				}
				RtlInitUnicodeString(&destpath, L"\\SeanXS_RedirectOut\0");
				RtlCopyUnicodeString(&pInstanceCtx->SrcDirPath, &destpath);

				sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &pInstanceCtx->DestDirPath);
				if (!NT_SUCCESS(sts))
				{
					break;
				}
				RtlInitUnicodeString(&destpath, L"\\SeanXS_RedirectIn\0");
				RtlCopyUnicodeString(&pInstanceCtx->DestDirPath, &destpath);

				sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &pInstanceCtx->DestDirVolume);
				if (!NT_SUCCESS(sts))
				{
					break;
				}
				RtlInitUnicodeString(&destpath, L"\\Device\\HarddiskVolume9\0");
				RtlCopyUnicodeString(&pInstanceCtx->DestDirVolume, &destpath);

				pInstanceCtx->RedirectOut = TRUE;
				pInstanceCtx->EntireVolume = FALSE;
			}
			else
			{

				//	测试context模块，不代表真正逻辑
				sts = FsFltAllocUnicodeString((USHORT)BufferSize, PagedPool, &pInstanceCtx->DestPrefix);
				if (!NT_SUCCESS(sts))
				{
					break;
				}
				RtlCopyUnicodeString(&pInstanceCtx->DestPrefix, &VolumeName);
				pInstanceCtx->RedirectIn = TRUE;
			}
			//	测试代码

			//KdPrint((__FUNCTION__"=> Volume Name : %wZ attached!\n", &VolumeName));
			DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, SEANXS_SFO_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE!=> Volume Name : %wZ attached.\n", &VolumeName);
			rtn = STATUS_SUCCESS;
		}
	} while (0);

	if (pInstanceCtx)
	{
		FltReleaseContext(pInstanceCtx);
	}

	FsFilterFreeUnicodeString(&VolumeName);

	//KdPrint((__FUNCTION__"=> Exit\n"));

	return rtn;
}


NTSTATUS
FsFilterInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This is called when an instance is being manually deleted by a
	call to FltDetachVolume or FilterDetach thereby giving us a
	chance to fail that detach request.

	If this routine is not defined in the registration structure, explicit
	detach requests via FltDetachVolume or FilterDetach will always be
	failed.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Indicating where this detach request came from.

Return Value:

	Returns the status of this operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	KdPrint((__FUNCTION__"=> Enter\n"));

	return STATUS_SUCCESS;
}


VOID
FsFilterInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This routine is called at the start of instance teardown.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Reason why this instance is being deleted.

Return Value:

	None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	KdPrint((__FUNCTION__"=> Enter\n"));

	return;
}


VOID
FsFilterInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This routine is called at the end of instance teardown.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Reason why this instance is being deleted.

Return Value:

	None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	KdPrint((__FUNCTION__"=> Enter\n"));

	return;
}