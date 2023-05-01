/*++

Module Name:

    seanxssfo.c

Abstract:

    This is the main module of the seanxs_sfo miniFilter driver.

Environment:

    Kernel mode

--*/

#include "Common.h"
#include "extern_def.h"
#include "Instance.h"
#include "OpCreate.h"
#include "OpClose.h"
#include "OpCleanUp.h"
#include "OpQueryInfo.h"
#include "OpSetInfo.h"

#include "Trace.h"
#include "seanxs_sfo.tmh"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle = NULL;
PDRIVER_OBJECT pDrvObj = NULL;
ULONG_PTR OperationStatusCtx = 1;
ULONG gTraceFlags = 0;// PTDBG_TRACE_REDIRECT_STATUS;

REGHANDLE hEtw = 0;

#ifdef USING_TRACELOGGING
TRACELOGGING_DECLARE_PROVIDER(g_hProvider);
TRACELOGGING_DEFINE_PROVIDER(
	g_hProvider,
	"seanxs_sfo",
	(0xa95d3f79, 0xabc2, 0x4b3c, 0xaf, 0x4a, 0x2d, 0x9c, 0x3, 0x96, 0x86, 0x54));
#endif

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
seanxssfoUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
seanxssfoPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
seanxssfoPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
seanxssfoPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, seanxssfoUnload)
#pragma alloc_text(PAGE, seanxssfoPreOperation)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
	  0,
	  OpPreCreateOperation,
	  OpPostCreateOperation },

	{ IRP_MJ_CLOSE,
	  0,
	  OpPreCloseOperation,
	  OpPostCloseOperation },

	{ IRP_MJ_CLEANUP,
	  0,
	  OpPreCleanUpOperation,
	  OpPostCleanUpOperation },

#if 1 // TODO - List all of the requests to filter.

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_READ,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_WRITE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      OpPreQueryInfoOperation,
      seanxssfoPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      OpPreSetInfoOperation,
      seanxssfoPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_SET_EA,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      seanxssfoPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_PNP,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      seanxssfoPreOperation,
      seanxssfoPostOperation },

#endif // TODO

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,	//  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    seanxssfoUnload,                           //  MiniFilterUnload

    seanxssfoInstanceSetup,                    //  InstanceSetup
    seanxssfoInstanceQueryTeardown,            //  InstanceQueryTeardown
    seanxssfoInstanceTeardownStart,            //  InstanceTeardownStart
    seanxssfoInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};




/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("seanxssfo!DriverEntry: Entered\n") );

	status = RegEtw(&hEtw);

	WPP_INIT_TRACING(DriverObject, RegistryPath);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_ENTRY, "%!FUNC!");
#ifdef USING_TRACELOGGING
	status = TraceLoggingRegister(g_hProvider);

	TraceLoggingWrite(
		g_hProvider,
		"MyDriverEntryEvent",
		TraceLoggingPointer(DriverObject),
		TraceLoggingUnicodeString(RegistryPath, "RegPath"));
#endif
    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

	KdPrint(("hello\r\n"));
	//status = EtwWriteString(hEtw, 1, 0xF, NULL, L"hello world\r\n\0");

	//EVENT_DESCRIPTOR evtDesc = { 0 };
	//EVENT_DATA_DESCRIPTOR evtDataDesc = { 0 };

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
			UnRegEtw(hEtw);
        }
    }

	if (NT_SUCCESS(status))
	{
		pDrvObj = DriverObject;
	}
	else
	{
		WPP_CLEANUP(DriverObject);
	}

    return status;
}

NTSTATUS
seanxssfoUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("seanxssfo!seanxssfoUnload: Entered\n") );

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UNLOAD,
		"%!FILE!,%!FUNC!,%!LINE! => FLT_FILTER_UNLOAD_FLAGS:%d\n",
		Flags);

    FltUnregisterFilter( gFilterHandle );

#ifdef USING_TRACELOGGING
	TraceLoggingUnregister(g_hProvider);
#endif

	WPP_CLEANUP(pDrvObj);
	UnRegEtw(hEtw);

    return STATUS_SUCCESS;
}

VOID
seanxssfoOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER(ParameterSnapshot);
	UNREFERENCED_PARAMETER(OperationStatus);
	UNREFERENCED_PARAMETER(RequesterContext);

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("seanxssfo!seanxssfoOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("seanxssfo!seanxssfoOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}

FLT_PREOP_CALLBACK_STATUS
seanxssfoPreOperationNoPostOperation (
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
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("seanxssfo!seanxssfoPreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
seanxssfoDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}

FLT_PREOP_CALLBACK_STATUS
seanxssfoPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
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

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CREATE,
            "%!FILE!,%!FUNC!,%!LINE! => TargetFileObject : %p, TargetInstance : %p, MajorFunction : %d, MinorFunction : %d\n",
            FltObjects->FileObject,
            Data->Iopb->TargetInstance,
            Data->Iopb->MajorFunction,
            Data->Iopb->MinorFunction);

        sts = STATUS_SUCCESS;
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
seanxssfoPostOperation(
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