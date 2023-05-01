#include "extern_def.h"
#include "OpCreate.h"
#include "CommonFuncs.h"

#include "Trace.h"
#include "OpCreate.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OpPreCreateOperation)
#endif

HANDLE ProcessFileTest = (HANDLE)9124;

/*************************************************************************
	MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
OpPreCreateOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	This routine is a pre-operation dispatch routine for this miniFilter.

	This is non-pageable because it could be called on the paging path

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
	FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;

	PAGED_CODE();

	if ((FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)) ||
		(FlagOn(Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN)) ||
		(FlagOn(Data->Iopb->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID))) {

		return ReturnValue;
	}

	*CompletionContext = NULL;

	do
	{
		if (FltObjects->Instance == DestInstance)
		{
			sts = STATUS_SUCCESS;
			break;
		}

		if (ProcessFileTest != PsGetCurrentProcessId())
		{
			sts = STATUS_SUCCESS;
			break;
		}

		sts = GetParsedNameInfo(Data, &pNameInfo);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CREATE,
				"%!FILE!,%!FUNC!,%!LINE! => GetParsedNameInfo failed : %x\n",
				sts);
			break;
		}

		if (!wcsstr(pNameInfo->Name.Buffer, SRC_PATH_NAME))
		{
			sts = STATUS_SUCCESS;
			break;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CREATE,
			"%!FILE!,%!FUNC!,%!LINE! => File Name : %wZ, TargetFileObject : %p, TargetInstance : %p\n",
			&pNameInfo->Name,
			FltObjects->FileObject,
			Data->Iopb->TargetInstance);

		sts = STATUS_SUCCESS;
		Data->IoStatus.Status = STATUS_SUCCESS;
		Data->IoStatus.Information = 0;
		ReturnValue = FLT_PREOP_COMPLETE;

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

	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
OpPostCreateOperation(
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

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("seanxssfo!seanxssfoPostOperation: Entered\n"));

	return FLT_POSTOP_FINISHED_PROCESSING;
}
