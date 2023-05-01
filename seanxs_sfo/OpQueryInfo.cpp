#include "OpQueryInfo.h"
#include "CommonFuncs.h"
#include "extern_def.h"

#include "Trace.h"
#include "OpQueryInfo.tmh"

FLT_PREOP_CALLBACK_STATUS
OpPreQueryInfoOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;

	PAGED_CODE();

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

		/*sts = GetParsedNameInfo(Data, &pNameInfo);
		if (!NT_SUCCESS(sts))
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUERYINFO,
				"%!FILE!,%!FUNC!,%!LINE! => GetParsedNameInfo failed : %x\n",
				sts);
			break;
		}

		if (!wcsstr(pNameInfo->Name.Buffer, SRC_PATH_NAME))
		{
			sts = STATUS_SUCCESS;
			break;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUERYINFO,
			"%!FILE!,%!FUNC!,%!LINE! => File Name : %wZ, TargetFileObject : %p, TargetInstance : %p\n",
			&pNameInfo->Name,
			FltObjects->FileObject,
			Data->Iopb->TargetInstance);*/
		switch (Data->Iopb->Parameters.QueryFileInformation.FileInformationClass)
		{
			case FileNameInformation:
			{
				PFILE_NAME_INFORMATION pFileNameInfo = NULL;

				pFileNameInfo = (PFILE_NAME_INFORMATION)Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
				pFileNameInfo->FileNameLength = 0x4E;
				RtlCopyMemory(pFileNameInfo->FileName, L"\\seanxs_sfo_redirectout\\test\\abcd.txt", 0x4A);
				Data->IoStatus.Information = 0x4E;
				Data->IoStatus.Status = STATUS_SUCCESS;

				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUERYINFO,
					"%!FILE!,%!FUNC!,%!LINE! => TargetFileObject : %p, TargetInstance : %p, FileNameInformation : %ws\n",
					FltObjects->FileObject,
					Data->Iopb->TargetInstance,
					L"\\seanxs_sfo_redirectout\\test\\abcd.txt");

				break;
			}
			default:
			{
				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUERYINFO,
					"%!FILE!,%!FUNC!,%!LINE! => TargetFileObject : %p, TargetInstance : %p, FileInformationClass : %d\n",
					FltObjects->FileObject,
					Data->Iopb->TargetInstance,
					Data->Iopb->Parameters.QueryFileInformation.FileInformationClass);

				Data->IoStatus.Information = 0;
				Data->IoStatus.Status = STATUS_NOT_SUPPORTED;

				break;
			}
		}

		ReturnValue = FLT_PREOP_COMPLETE;
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

	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
OpPostQueryInfoOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("seanxssfo!seanxssfoPostOperation: Entered\n"));

	return FLT_POSTOP_FINISHED_PROCESSING;
}