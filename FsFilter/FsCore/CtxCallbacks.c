/*++
Module Name:

	CtxCallBacks.c

Abstract:

	创建，使用，关闭各种类型的context

Environment:
	Kernel Mode

--*/

#include "CtxCallbacks.h"
#include "..\FsCommon\CommonFunctions.h"
#include "..\FsCommon\Extern_def.h"

//注册各种上下文结构
const FLT_CONTEXT_REGISTRATION CtxReg[] =
{
	{
		FLT_INSTANCE_CONTEXT,
		0,
		FsFilterInstanceCtxCleanup,
		sizeof(FSFILTER_INSTANCE_CONTEXT),
		FSFILTER_CONTEXT_TAG,
		NULL,
		NULL,
		NULL
	},

	{
		FLT_FILE_CONTEXT,
		0,
		FsFltFileCtxCleanup,
		sizeof(FSFILTER_FILE_CONTEXT),
		FSFILTER_CONTEXT_TAG,
		NULL,
		NULL,
		NULL
	},

	{
		FLT_STREAMHANDLE_CONTEXT,
		0,
		FsFilterSteamHandleCtxCleanup,
		sizeof(FSFILTER_STEAMHANDLE_CONTEXT),
		FSFILTER_CONTEXT_TAG,
		NULL,
		NULL,
		NULL
	},

	{   FLT_TRANSACTION_CONTEXT,
		0,
		FsFltTrxCtxCleanup,
		sizeof(FSFILTER_TRX_CONTEXT),
		FSFILTER_CONTEXT_TAG,
		NULL,
		NULL,
		NULL
	},

	{FLT_CONTEXT_END}
};

VOID FsFilterSteamHandleCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
)
/*++

Routine Description:

	清理FLT_STREAMHANDLE_CONTEXT类型的Context

Arguments:

	pContext - Context指针
	ContextType - Context类型

Return Value:

	无

--*/
{
	UNREFERENCED_PARAMETER(ContextType);

	if (Context == NULL)
	{
		return;
	}

	KdPrint((__FUNCTION__"=> Context : %p Clean up\n", Context));
}

VOID FsFilterInstanceCtxCleanup(
	_In_ PFLT_CONTEXT pContext,
	_In_ FLT_CONTEXT_TYPE ContextType
)
/*++

Routine Description:

	清理FLT_INSTANCE_CONTEXT类型的Context

Arguments:

	pContext - Context指针
	ContextType - Context类型

Return Value:

	无

--*/
{
	FLT_ASSERT(ContextType == FLT_INSTANCE_CONTEXT);
	UNREFERENCED_PARAMETER(ContextType);
	
	if (pContext == NULL)
	{
		return;
	}

	KdPrint((__FUNCTION__"=> Context : %p Clean up\n", pContext));

	PFSFILTER_INSTANCE_CONTEXT pInstCtx = (PFSFILTER_INSTANCE_CONTEXT)pContext;
	
	FsFilterFreeUnicodeString(&pInstCtx->DestPrefix);
	FsFilterFreeUnicodeString(&pInstCtx->SrcDirPath);
	FsFilterFreeUnicodeString(&pInstCtx->DestVolume);

	return;
}

NTSTATUS FsFilterInstanceCtxCreate(
	_In_ PFLT_INSTANCE pInstance,
	_Out_ PFSFILTER_INSTANCE_CONTEXT * pContext
)
/*++

Routine Description:

	创建FLT_INSTANCE_CONTEXT类型的Context

Arguments:

	pInstance - Context所属Instance的指针
	pContext - 保存Context的指针，可以null,不是null时，调用者需要FltReleaseContext

Return Value:

	无

--*/
{
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFSFILTER_INSTANCE_CONTEXT pCtxTemp = NULL;

	if (pInstance == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		//	分配context
		sts = FltAllocateContext(
			gFilterHandle,
			FLT_INSTANCE_CONTEXT,
			sizeof(FSFILTER_INSTANCE_CONTEXT),
			NonPagedPool,
			&pCtxTemp);

		if (!NT_SUCCESS(sts))
		{
			KdPrint(("FltAllocateContext() failed, status : %08X\n", sts));
			break;
		}

		//	将context与instance关联
		sts = FltSetInstanceContext(
			pInstance,
			FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
			(PFLT_CONTEXT)pCtxTemp,
			NULL);

		if (!NT_SUCCESS(sts))
		{
			KdPrint(("FltAllocateContext() failed, status : %08X\n", sts));
			break;
		}

		RtlZeroMemory(pCtxTemp, sizeof(FSFILTER_INSTANCE_CONTEXT));

		//	返回context指针给调用者
		if (pContext != NULL)
		{
			*pContext = pCtxTemp;
		}
		else
		{
			//	释放这次引用
			FltReleaseContext((PFLT_CONTEXT)pCtxTemp);
		}
	} while (0);		

	return sts;
}

VOID FsFltFileCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
)
/*++

Routine Description:

	清理FLT_FILE_CONTEXT类型的Context

Arguments:

	pContext - Context指针
	ContextType - Context类型

Return Value:

	无

--*/
{
	UNREFERENCED_PARAMETER(ContextType);
	FLT_ASSERT(ContextType == FLT_FILE_CONTEXT);

	PFSFILTER_FILE_CONTEXT pFileCtx = (PFSFILTER_FILE_CONTEXT)Context;
	
	pFileCtx->m_bIsReparsed = FALSE;

	KdPrint((__FUNCTION__"=> File Name : %wZ\n", &pFileCtx->OriginalFileName));

	FsFilterFreeUnicodeString(&pFileCtx->OriginalFileName);
	
	return;
}

NTSTATUS ApplFltCreateFileCtx(
	_In_ PFLT_INSTANCE pInstance,
	_In_ PFILE_OBJECT pFileObj,
	_Inout_ PFSFILTER_FILE_CONTEXT * pContext
)
/*++

Routine Description:

	新建FLT_FILE_CONTEXT类型的Context

Arguments:

	pInstance - Context所属Instance的指针
	pContext - 保存Context的指针，不可以null，调用者需要FltReleaseContext
	pFileobj - context所属的文件对象

Return Value:

	无

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	if (pInstance == NULL ||
		pFileObj == NULL ||
		pContext == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	PFSFILTER_FILE_CONTEXT pFileCtxNew = NULL;

	if (!FltSupportsFileContexts(pFileObj))
	{
		return STATUS_NOT_SUPPORTED;
	}

	rtn = FltGetFileContext(pInstance, pFileObj, pContext);
	if (NT_SUCCESS(rtn))
	{
		return rtn;
	}

	rtn = FltAllocateContext(
		gFilterHandle,
		FLT_FILE_CONTEXT,
		sizeof(FSFILTER_FILE_CONTEXT),
		NonPagedPool,
		&pFileCtxNew);
	if (NT_SUCCESS(rtn))
	{
		rtn = FltSetFileContext(
			pInstance,
			pFileObj,
			FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
			pFileCtxNew,
			NULL);
		
		if (NT_SUCCESS(rtn))
		{
			RtlZeroMemory(pFileCtxNew, sizeof(FSFILTER_FILE_CONTEXT));
			*pContext = pFileCtxNew;
		}
	}

	return rtn;
}

NTSTATUS ApplFltGetFileCtx(
	_In_ PFLT_INSTANCE pInstance,
	_In_ PFILE_OBJECT pFileObj,
	_Inout_ PFSFILTER_FILE_CONTEXT * pContext
)
/*++

Routine Description:

	获取FLT_FILE_CONTEXT类型的Context

Arguments:

	pInstance - Context所属Instance的指针
	pContext - 保存Context的指针，不可以null，调用者需要FltReleaseContext
	pFileobj - context所属的文件对象

Return Value:

	无

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;
	
	if (pInstance == NULL ||
		pFileObj == NULL ||
		pContext == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (!FltSupportsFileContexts(pFileObj))
	{
		return STATUS_NOT_SUPPORTED;
	}

	rtn = FltGetFileContext(pInstance, pFileObj, pContext);
	
	return rtn;
}

VOID FsFltTrxCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
)
/*++

Routine Description:

	清理FLT_TRANSACTION_CONTEXT类型的Context

Arguments:

	pContext - Context指针
	ContextType - Context类型

Return Value:

	无

--*/
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(ContextType);

	return;
}