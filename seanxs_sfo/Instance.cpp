#include "Instance.h"
#include "CommonFuncs.h"

#include "Trace.h"
#include "Instance.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, seanxssfoInstanceQueryTeardown)
#pragma alloc_text(PAGE, seanxssfoInstanceSetup)
#pragma alloc_text(PAGE, seanxssfoInstanceTeardownStart)
#pragma alloc_text(PAGE, seanxssfoInstanceTeardownComplete)
#endif

PFLT_INSTANCE SrcInstance = NULL;
PFLT_INSTANCE DestInstance = NULL;

#define SRC_VOLUME_NAME L"\\Device\\HarddiskVolume7"
#define DEST_VOLUME_NAME L"\\Device\\HarddiskVolume10"

UNICODE_STRING SrcVolume = { 0x2e,0x2e, SRC_VOLUME_NAME };
UNICODE_STRING DestVolume = { 0x30, 0x30, DEST_VOLUME_NAME };

NTSTATUS
seanxssfoInstanceQueryTeardown(
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
	PAGED_CODE();

	UNICODE_STRING VolumeName = { 0 };
	NTSTATUS sts = STATUS_UNSUCCESSFUL;

	do
	{
		sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &VolumeName);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FsFltAllocUnicodeString:%x\n",
				sts);
			break;
		}

		sts = FltGetVolumeName(FltObjects->Volume, &VolumeName, NULL);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FltGetVolumeName:%x\n",
				sts);
			break;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
			"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS:%d\n",
			&VolumeName,
			Flags);

	} while (0);

	FsFilterFreeUnicodeString(&VolumeName);

	return STATUS_SUCCESS;
}


VOID
seanxssfoInstanceTeardownStart(
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
	UNICODE_STRING VolumeName = { 0 };
	NTSTATUS sts = STATUS_UNSUCCESSFUL;

	PAGED_CODE();

	do
	{
		sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &VolumeName);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FsFltAllocUnicodeString:%x\n",
				sts);
			break;
		}

		sts = FltGetVolumeName(FltObjects->Volume, &VolumeName, NULL);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FltGetVolumeName:%x\n",
				sts);
			break;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
			"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ, FLT_INSTANCE_TEARDOWN_FLAGS:%d\n",
			&VolumeName,
			Flags);

	} while (0);

	FsFilterFreeUnicodeString(&VolumeName);

	return;
}


VOID
seanxssfoInstanceTeardownComplete(
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
	UNICODE_STRING VolumeName = { 0 };
	NTSTATUS sts = STATUS_UNSUCCESSFUL;

	PAGED_CODE();

	do
	{
		sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &VolumeName);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FsFltAllocUnicodeString:%x\n",
				sts);
			break;
		}

		sts = FltGetVolumeName(FltObjects->Volume, &VolumeName, NULL);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FltGetVolumeName:%x\n",
				sts);
			break;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
			"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ, FLT_INSTANCE_TEARDOWN_FLAGS:%d\n",
			&VolumeName,
			Flags);

	} while (0);

	FsFilterFreeUnicodeString(&VolumeName);

	return;
}


NTSTATUS
seanxssfoInstanceSetup(
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
	UNICODE_STRING VolumeName = { 0 };
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	NTSTATUS rtn = STATUS_FLT_DO_NOT_ATTACH;

	PAGED_CODE();

	if (FLT_FSTYPE_MUP == VolumeFilesystemType ||
		FLT_FSTYPE_RAW == VolumeFilesystemType ||
		FLT_FSTYPE_CDFS == VolumeFilesystemType ||
		FLT_FSTYPE_UDFS == VolumeFilesystemType)
	{
		return rtn;
	}

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("seanxssfo!seanxssfoInstanceSetup: Entered\n"));

	do
	{
		sts = FsFltAllocUnicodeString((USHORT)256, PagedPool, &VolumeName);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FsFltAllocUnicodeString:%x\n",
				sts);
			break;
		}

		sts = FltGetVolumeName(FltObjects->Volume, &VolumeName, NULL);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => FltGetVolumeName:%x\n",
				sts);
			break;
		}

		//
		//  Prevent the filter from auto attaching.
		//

		if (FlagOn(Flags, FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT))
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ not attached! VolumeDeviceType : %d, VolumeFilesystemType : %d, FLT_INSTANCE_SETUP_FLAGS:%d\n",
				&VolumeName,
				VolumeDeviceType,
				VolumeFilesystemType,
				Flags);
			break;
		}

		if (FlagOn(Flags, FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT))
		{
			rtn = STATUS_SUCCESS;

			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ being attached! VolumeDeviceType : %d, VolumeFilesystemType : %d, FLT_INSTANCE_SETUP_FLAGS:%d\n",
				&VolumeName,
				VolumeDeviceType,
				VolumeFilesystemType,
				Flags);
			
			if (RtlCompareUnicodeString(&VolumeName, &SrcVolume, FALSE) == 0)
			{
				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
					"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ attached as source volume! VolumeDeviceType : %d, VolumeFilesystemType : %d, FLT_INSTANCE_SETUP_FLAGS:%d\n",
					&VolumeName,
					VolumeDeviceType,
					VolumeFilesystemType,
					Flags);
				SrcInstance = FltObjects->Instance;
				break;
			}
			else if (RtlCompareUnicodeString(&VolumeName, &DestVolume, FALSE) == 0)
			{
				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
					"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ attached as destination volume! VolumeDeviceType : %d, VolumeFilesystemType : %d, FLT_INSTANCE_SETUP_FLAGS:%d\n",
					&VolumeName,
					VolumeDeviceType,
					VolumeFilesystemType,
					Flags);
				DestInstance = FltObjects->Instance;
				break;
			}

			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
				"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ attached in passthrough mode! VolumeDeviceType : %d, VolumeFilesystemType : %d, FLT_INSTANCE_SETUP_FLAGS:%d\n",
				&VolumeName,
				VolumeDeviceType,
				VolumeFilesystemType,
				Flags);
		}
		
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INSTANCE,
			"%!FILE!,%!FUNC!,%!LINE! => Volume : %wZ not attached! VolumeDeviceType : %d, VolumeFilesystemType : %d, FLT_INSTANCE_SETUP_FLAGS:%d\n",
			&VolumeName,
			VolumeDeviceType,
			VolumeFilesystemType,
			Flags);

	} while (0);

	FsFilterFreeUnicodeString(&VolumeName);

	return rtn;
}
