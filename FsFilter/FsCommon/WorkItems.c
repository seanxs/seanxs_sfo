/*++
Module Name:

	WorkItems.c

Abstract:

	需要在WorkItem环境执行的函数
	拷贝文件
	创建目录
	……

Environment:
	Kernel Mode

--*/

#include "WorkItems.h"
#include "CommonFunctions.h"
#include "..\FsCore\CtxCallbacks.h"

VOID WorkItem_CreateDir(
	_In_ PFLT_DEFERRED_IO_WORKITEM FltWorkItem,
	_In_ PFLT_CALLBACK_DATA CallbackData,
	_In_opt_ PVOID Context
)
/*++

Routine Description:

	在PostCreate回调函数中调用
	如果请求的File在源volume里存在,复制到重定向后的volume
	如果File类型是目录，只新建这层目录(可支持多层)，这个目录内的子目录和文件，不复制
	如果File类型是文件，重定向到源volume或其它volume

Arguments:

	FltWorkItem - Work item类型指针
	CallbackData - 请求的回调数据
	Context - 自定义的context

Return Value:

	无

--*/
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	PFILE_OBJECT pFileObj = NULL;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	FILE_STANDARD_INFORMATION FileStdInfo = { 0 };
	BOOLEAN IsDir = FALSE;

	//KdPrint((__FUNCTION__"=> enter\n"));

	PFSFILTER_WORKITEMCTX_CREATEDIR pCreateDirCtx = (PFSFILTER_WORKITEMCTX_CREATEDIR)Context;

	switch (pCreateDirCtx->PreOrPost)
	{
	case WORKITEM_PREORPOST_PRECALLBACK:
	{
		KdPrint((__FUNCTION__"=> From PreCallBack\n"));
		FltCompletePendedPreOperation(CallbackData, FLT_PREOP_SUCCESS_NO_CALLBACK, NULL);
		break;
	}
	case WORKITEM_PREORPOST_POSTCALLBACK:
	{
		//KdPrint((__FUNCTION__"=> From PostCallBack\n"));

		do
		{
			//	判断文件是否存在
			sts = ApplFsOpenFile(
				&pCreateDirCtx->pEcpCtx->OriginalFileName,
				pCreateDirCtx->pEcpCtx->pOriginalInstance,
				&hFile,
				&pFileObj);
			if (!NT_SUCCESS(sts))
			{
				if (sts == STATUS_OBJECT_NAME_NOT_FOUND || sts == STATUS_OBJECT_PATH_NOT_FOUND)
				{
					//KdPrint((__FUNCTION__"=> file : %wZ does not exist at original\n", pCreateDirCtx->pEcpCtx->OriginalFileName));
				}
				break;
			}

			//	判断是不是目录
			sts = FltIsDirectory(pFileObj, pCreateDirCtx->pEcpCtx->pOriginalInstance, &IsDir);
			if (!NT_SUCCESS(sts))
			{
				break;
			}

			if (FileStdInfo.Directory)
			{
				KdPrint((__FUNCTION__"=> file : %wZ is a Directory, need to be created\n", pCreateDirCtx->pEcpCtx->OriginalFileName));
			}
			else
			{
				KdPrint((__FUNCTION__"=> file : %wZ is not Directory, will not be created\n", pCreateDirCtx->pEcpCtx->OriginalFileName));
			}

		} while (0);

		if (pFileObj != NULL)
		{
			ObDereferenceObject(pFileObj);
		}

		if (hFile != INVALID_HANDLE_VALUE)
		{
			FltClose(hFile);
		}

		break;
	}
	default:
		break;
	}

	FltCompletePendedPostOperation(CallbackData);
	FltFreeDeferredIoWorkItem(FltWorkItem);
	ExFreePoolWithTag(Context, 'twic');

	return;
}

VOID WorkItem_SetSecurity(
	_In_ PFLT_DEFERRED_IO_WORKITEM FltWorkItem,
	_In_ PFLT_CALLBACK_DATA CallbackData,
	_In_opt_ PVOID Context
)
/*++

Routine Description:

	远端文件的security已被改变，设置本地文件的security

Arguments:

	FltWorkItem - Work item类型指针
	CallbackData - 请求的回调数据
	Context - 自定义的context

Return Value:

	无

--*/
{
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	PFILE_OBJECT pFileObj = NULL;
	PFSFILTER_WORKITEMCTX_SETSECURITY pWorkItemCtxSetSecurity = (PFSFILTER_WORKITEMCTX_SETSECURITY)Context;

	do
	{
		//	判断文件是否存在
		

		//	本地文件存在，执行操作
		//	Todo

		sts = STATUS_SUCCESS;

	} while (0);

	if (pFileObj != NULL)
	{
		ObDereferenceObject(pFileObj);
	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		FltClose(hFile);
	}

	FltCompletePendedPostOperation(CallbackData);
	FltFreeDeferredIoWorkItem(FltWorkItem);
	ExFreePoolWithTag(pWorkItemCtxSetSecurity, 'twic');

	return;
}