/*++

Module Name:

    FsFilter.c

Abstract:

    miniFilter驱动的主模块文件。

Environment:

    Kernel mode

--*/

#include "FsFilter.h"

#include "FsCore/OpCreate.h"
#include "FsCore/OpDirCtrl.h"
#include "FsCore/OpCleanUp.h"
#include "FsCore/OpClose.h"
#include "FsCore/OpSetInformation.h"
#include "FsCore/OpQueryInfo.h"
#include "FsCore/OpWrite.h"
#include "FsCore/OpSetSecurity.h"
#include "FsCore/OpNetworkQueryOpen.h"
#include "FsCore/Instance.h"
#include "FsCore/Communication.h"
#include "FsCore/CtxCallbacks.h"
#include "FsCore/OpQueryVolume.h"
#include "FsCore/Ecp.h"
#include "FsCore/Ktm.h"
#include "nameprovider.h"
#include "FsCommon/Extern_def.h"
#include "FsCommon/KernelObjs.h"

#include "FsCommon/CommonFunctions.h"
#include "FsCommon/Trace.h"

#include "FsFilter.tmh"

//#include "FsCommon/GetConfig.h"
//#include "FsCommon/PathParser.h"
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PDRIVER_OBJECT pDrvObj = NULL;
PFLT_FILTER gFilterHandle = NULL;	// 当前过滤驱动的句柄

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0xffffffff;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FsFilterUnload)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
	  FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
	  FsFilterPreCreate,
	  FsFilterPostCreate },

	{ IRP_MJ_QUERY_INFORMATION,
	  0,
	  FsFilterPreQueryInfo,
	  FsFilterPostQueryInfo },

	{ IRP_MJ_DIRECTORY_CONTROL,
      0,
	  FsFilterPreDirCtrl,
	  FsFilterPostDirCtrl },

	{ IRP_MJ_SET_INFORMATION,
	  FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
	  FsFilterPreSetInformation,
	  FsFilterPostSetInformation },
	
	{ IRP_MJ_WRITE,
	  0,
	  FsFilterPreWrite,
	  FsFilterPostWrite },

	{ IRP_MJ_CLOSE,
	  0,
	  FsFilterPreClose,
	  FsFilterPostClose },

	{ IRP_MJ_CLEANUP,
	  0,
	  FsFilterPreCleanUp,
	  FsFilterPostCleanUp },

	{ IRP_MJ_SET_SECURITY,
	  0,
	  FsFilterPreSetSecurity,
	  FsFilterPostSetSecurity },

	{ IRP_MJ_QUERY_VOLUME_INFORMATION,
	  0,
	  FsFilterPreQueryVolume,
	  FsFilterPostQueryVolume },

	 { IRP_MJ_NETWORK_QUERY_OPEN,
	  FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
	  FsFilterPreNetworkQueryOpen,
	  FsFilterPostNetworkQueryOpen },


	{ IRP_MJ_OPERATION_END }

#if 0 // TODO - List all of the requests to filter.

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_READ,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_SET_EA,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      FsFilterPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_PNP,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      FsFilterPreOperation,
      FsFilterPostOperation },

#endif // TODO

};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
#ifdef SUPPORT_UNLOAD_DRIVER
	FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,
#else
	0,                                  //  Flags
#endif
	CtxReg,                               //  Context
    Callbacks,                          //  Operation callbacks
#ifdef SUPPORT_UNLOAD_DRIVER
    FsFilterUnload,                           //  MiniFilterUnload
#else
	NULL,
#endif
    FsFilterInstanceSetup,                    //  InstanceSetup
    FsFilterInstanceQueryTeardown,            //  InstanceQueryTeardown
    FsFilterInstanceTeardownStart,            //  InstanceTeardownStart
    FsFilterInstanceTeardownComplete,         //  InstanceTeardownComplete

	FsFilterGeneFileName,                               //  GenerateFileName
	FsFilterNormalizeNameComponent,                                //  NormalizeNameComponent
	NULL,	//	NormalizeContextCleanupCallback
	TransactionNtfCb,	//	TransactionNotificationCallback
	NULL,	//	NormalizeNameComponentExCallback
#if FLT_MGR_WIN8
	NULL	//	SectionNotificationCallback
#endif
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

    驱动程序的入口函数

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

    //UNREFERENCED_PARAMETER( RegistryPath );

	do
	{
		//	初始化日志模块
		WPP_INIT_TRACING(DriverObject, RegistryPath);
		//KdPrint((__FUNCTION__"=>\n"));
		//DoTraceMessage(SEANXS_SFO_GENERAL, "seanxs shadow file object Driver Sample!DoTraceMessage\n");
		//DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, SEANXS_SFO_GENERAL, "seanxs shadow file object Driver Sample!DoTraceLevelMessage\n");
		MyTrace(TRACE_LEVEL_VERBOSE, SEANXS_SFO_GENERAL, "%!FUNC!()=>\n");

		//	初始化驱动需要的各种Kernel Object
		status = InitKernelObjs();
		if (!NT_SUCCESS(status))
		{
			break;
		}

		//
		//  Register with FltMgr to tell it our callback routines
		//

		status = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		//	创建通讯端口
		status = EstablishCommunicate(AppLFsFilterPortName, gFilterHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		//  Start filtering i/o
		status = FltStartFiltering(gFilterHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		//	初始化ecp所需资源
		InitEcpStaff();

		pDrvObj = DriverObject;

		//	到这里，所有准备工作都成功完成
		rtn = STATUS_SUCCESS;
	} while (0);
	
	//	驱动初始化没有成功，执行各种清理工作
	if (!NT_SUCCESS(rtn))
	{
		//	清理kernel Object
		CleanupKernelObjs();

		//	关闭通讯
		CloseCommunicate();

		if (gFilterHandle)
		{
			//	注销驱动
			FltUnregisterFilter(gFilterHandle);
			gFilterHandle = NULL;
		}

		//	关闭日志
		//CloseLog();
		WPP_CLEANUP(DriverObject);
	}

    return rtn;
}

NTSTATUS
FsFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    驱动程序卸载函数

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

	KdPrint((__FUNCTION__"=>\n"));

	//	关闭通讯
	CloseCommunicate();

	//	释放ecp所需资源
	CleanUpEcpStaff();

	if (gFilterHandle)
	{
		//	注销驱动
		FltUnregisterFilter(gFilterHandle);
		gFilterHandle = NULL;
	}

	//	清理kernel Object
	CleanupKernelObjs();

	//	关闭日志
	//CloseLog();
	if (pDrvObj)
	{
		WPP_CLEANUP(pDrvObj);
	}

    return STATUS_SUCCESS;
}


