/*++
Module Name:

	OpCreate.c

Abstract:

	处理IRP_MJ_CREATE请求
	基于minifilter技术

Environment:
	Kernel Mode

--*/

#include "OpCreate.h"
#include "CtxCallbacks.h"
#include "Ecp.h"
#include "..\FsCommon\Extern_def.h"
#include "..\FsCommon\CommonFunctions.h"
#include "..\FsCommon\WorkItems.h"
#include "../FsCommon/PathParser.h"

#include "..\FsCommon\Trace.h"
#include "OpCreate.tmh"

FLT_PREOP_CALLBACK_STATUS
FsFilterPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	IRP_MJ_CREATE的Pre回调函数

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

	1. 有ECP标记，说明经过reparse处理
	2. 完整路径字串作为参数，调用重定向策略接口函数
	3. 重定向策略接口函数返回是否需要重定向，以及新的路径字串
	4. 不需要重定向，返回FLT_PREOP_SUCCESS_NO_CALLBACK
	5. 需要重定向，返回FLT_PREOP_COMPLETE
	6. 有错误发生，返回FLT_PREOP_COMPLETE
	7. 已经过重定向，返回FLT_PREOP_SUCCESS_WITH_CALLBACK
	
--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFSFILTER_INSTANCE_CONTEXT pInstanceCtx = NULL;
	PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;
	PFSFLT_REPARSE_INFO_ECP_CONTEXT pEcpContext = NULL;

	PAGED_CODE();

	do
	{
		//
		//  Check if this open is to the paging file.
		//  We are not going to handle munging the namespace for paging files.
		//

		if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)) {

			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			break;
		}

		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_DISALLOW_FASTIO;
			break;
		}

		//
		//  Check if this is a volume open.
		//  Volume opens do not affect the namespace of the volume so we don't care.
		//

		if (FlagOn(Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN)) {

			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			break;
		}

		//
		//  Check if this open is by ID.
		//  Opens by file ID are name agnostic. Thus we do not care about this open.
		//

		if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID)) {

			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			break;
		}

		//	测试代码
		sts = FltGetInstanceContext(FltObjects->Instance, (PFLT_CONTEXT)&pInstanceCtx);
		if (sts == STATUS_NOT_FOUND)
		{
			//	没有找到context，说明这个volume里面的全部操作，都不需要重定向
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			break;
		}
		else if (!NT_SUCCESS(sts))
		{
			//	异常错误
			ReturnValue = FLT_PREOP_COMPLETE;
			break;
		}

		//	检查ECP标记，如果存在说明这次Create操作已经过reparse处理
		//	返回FLT_PREOP_SUCCESS_WITH_CALLBACK
		sts = GetECP(Data, FltObjects, &pEcpContext);
		if (NT_SUCCESS(sts) && pEcpContext != NULL)
		{
			//	已经过reparse处理
			//KdPrint((__FUNCTION__"=> File : %wZ already reparsed\n", Data->Iopb->TargetFileObject->FileName));
			MyTrace(TRACE_LEVEL_INFORMATION, SEANXS_SFO_CREATE, __FUNCTION__"=> GetECP succeed!\n");
			ReturnValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			break;
		}
		else if (sts == STATUS_NOT_FOUND)
		{
			//	没有ecp，说明没有reparse
			sts = STATUS_SUCCESS;
		}

		//	判断重定向，获取Instance的context，是否包含需要重定向的目录或文件
		if (pInstanceCtx->RedirectOut)
		{
			//KdPrint(("neet to be redirect\n"));
			//	获取文件名信息
			sts = GetParsedNameInfo(Data, &pNameInfo);
			if (NT_SUCCESS(sts))
			{
				if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
				{
					KdPrint((__FUNCTION__"=> Directory : %wZ, DesiredAccess : 0x%X\n",
						&pNameInfo->Name,
						Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess));
					//ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
					//break;
				}

				/*ACCESS_MASK DesiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
				if (FlagOn(DesiredAccess, FILE_ADD_FILE) ||
					FlagOn(DesiredAccess, FILE_LIST_DIRECTORY) ||
					FlagOn(DesiredAccess, FILE_ADD_SUBDIRECTORY))
				{
					KdPrint((__FUNCTION__"=> Directory : %wZ\n"), pNameInfo->Name);
				}*/

				//	判断是否这个volume都要重定向
				if (!pInstanceCtx->EntireVolume)
				{
					DoTraceLevelMessage(
						TRACE_LEVEL_INFORMATION,
						SEANXS_SFO_CREATE,
						"%!FILE!,%!FUNC!,%!LINE!=> File Name : %wZ, Create.Options : %X\n",
						&pNameInfo->Name,
						Data->Iopb->Parameters.Create.Options);
					//	文件路径作为参数，调用重定向策略接口函数
					//	此时，用比较字符串的方式暂时代替重定向策略
					if (wcsstr(pNameInfo->Name.Buffer, pInstanceCtx->SrcDirPath.Buffer))
					{
						UNICODE_STRING destpath = { 0 };
						sts = FsFltAllocUnicodeString((USHORT)1024, PagedPool, &destpath);
						if (!NT_SUCCESS(sts))
						{
							return sts;
						}

						sts = FsFilterSetReparsePath(Data, &destpath, pInstanceCtx);

						sts = FileReparseOperation(Data, destpath, FltObjects);

						DoTraceLevelMessage(
							TRACE_LEVEL_INFORMATION,
							SEANXS_SFO_CREATE,
							"%!FILE!,%!FUNC!,%!LINE!=> File Name : %wZ(%p) will be redirected to : %wZ\n",
							&pNameInfo->Name,
							FltObjects->FileObject,
							&destpath);

						//	添加ECP标记
						//	设置ecp参数
						AddECP(Data, FltObjects, &pEcpContext);
						if (!NT_SUCCESS(sts) || pEcpContext == NULL)
						{
							break;
						}
						else
						{
							pEcpContext->IsReparsed = TRUE;
							pEcpContext->pOriginalInstance = FltObjects->Instance;
							pEcpContext->pOriginalVolume = FltObjects->Volume;
							FsFltCopyUnicodeString(&pEcpContext->OriginalFileName, &pNameInfo->Name);
						}

						//	返回FLT_PREOP_COMPLETE
						if (NT_SUCCESS(sts))
						{
							ReturnValue = FLT_PREOP_COMPLETE;
						}

						FsFilterFreeUnicodeString(&destpath);
						break;
					}
					else
					{
						//	不需要重定向
						//KdPrint((__FUNCTION__"=> File : %wZ will not be reparsed\n", &pNameInfo->Name));
						ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
						break;
					}
				}
				else
				{
					UNICODE_STRING destpath = { 0 };
					sts = FsFltAllocUnicodeString((USHORT)1024, PagedPool, &destpath);
					if (!NT_SUCCESS(sts))
					{
						return sts;
					}

					//	整个卷都要重定向，判断是否有单独需要重定向到其它地方的路径
					if (wcsstr(pNameInfo->Name.Buffer, L"\\RJUPM\0"))
					{
						//	RJUPM目录重定向到\Device\HarddiskVolume10
						RtlAppendUnicodeToString(&destpath, L"\\Device\\HarddiskVolume10");
						RtlAppendUnicodeStringToString(&destpath, &(Data->Iopb->TargetFileObject->FileName));

						sts = FileReparseOperation(Data, destpath, FltObjects);
					}
					else// if(pInstanceCtx->EntireVolume)
					{
						//	这个卷重定向，替换volumename，添加路径前缀						
						sts = FsFilterSetReparsePath(Data, &destpath, pInstanceCtx);
						sts = FileReparseOperation(Data, destpath, FltObjects);

					}
					/*else
					{
						sts = STATUS_SUCCESS;
						ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
						break;
					}*/

					//	添加ECP标记
					//	设置ecp参数
					sts = AddECP(Data, FltObjects, &pEcpContext);
					if (!NT_SUCCESS(sts) || pEcpContext == NULL)
					{
						break;
					}
					else
					{
						pEcpContext->IsReparsed = TRUE;
						pEcpContext->pOriginalInstance = FltObjects->Instance;
						pEcpContext->pOriginalVolume = FltObjects->Volume;
						FsFltCopyUnicodeString(&pEcpContext->OriginalFileName, &pNameInfo->Name);
					}

					//	返回FLT_PREOP_COMPLETE
					if (NT_SUCCESS(sts))
					{
						ReturnValue = FLT_PREOP_COMPLETE;
					}

					FsFilterFreeUnicodeString(&destpath);
					break;
				}
			}
			else
			{
				KdPrint((__FUNCTION__"=> GetParsedNameInfo failed\n"));
				ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
			}
		}
		else
		{
			//	不做重定向
			//KdPrint(("not need to be redirected\n"));
			ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
		}
		//	测试代码

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

	if (pInstanceCtx)
	{
		FltReleaseContext(pInstanceCtx);
	}

	return ReturnValue;
}

FLT_POSTOP_CALLBACK_STATUS
FsFilterPostCreate(
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
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;
	PFSFLT_REPARSE_INFO_ECP_CONTEXT pEcpContext = NULL;
	UCHAR CreateDisposition = (UCHAR)(Data->Iopb->Parameters.Create.Options >> 24);
	ULONG CreateOption = (ULONG)(Data->Iopb->Parameters.Create.Options & 0xFFFFFF);
	PFLT_DEFERRED_IO_WORKITEM pFltDeferWorkItem = NULL;
	BOOLEAN IsDir = FALSE;

	do
	{
		if (Flags == FLTFL_POST_OPERATION_DRAINING)
		{
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		//	获取ecp参数
		sts = GetECP(Data, FltObjects, &pEcpContext);
		if (!NT_SUCCESS(sts) || pEcpContext == NULL)
		{
			//	没有经过reparse，直接让fltmgr继续处理
			sts = STATUS_SUCCESS;
			ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;
			break;
		}

		if (Data->IoStatus.Status == STATUS_OBJECT_NAME_NOT_FOUND ||
			Data->IoStatus.Status == STATUS_OBJECT_PATH_NOT_FOUND)
		{
			//	文件在这个volume上不存在，可能需要再次重定向到另一个volume,或者从源路径复制再次打开
			//KdPrint((__FUNCTION__"=> File : %wZ does not exist\n", pEcpContext->OriginalFileName));
			KdPrint((__FUNCTION__"=> File : %wZ does not exist, CreateOption : %x, CreateDisposition : %x\n",
				&Data->Iopb->TargetFileObject->FileName,
				CreateOption,
				CreateDisposition));

			//WriteLogToEtw(L"=> File does not exist\n\0");

			//	如果在源volume内存在，是否需要复制到当前volume
			if (BooleanFlagOn(CreateOption, FILE_DIRECTORY_FILE))
			{
				KdPrint((__FUNCTION__"=> File : %wZ is a directory\n", &Data->Iopb->TargetFileObject->FileName));
			}

			if (BooleanFlagOn(CreateDisposition, FILE_EXISTS))
			{

			}
			
			pFltDeferWorkItem = FltAllocateDeferredIoWorkItem();
			if (pFltDeferWorkItem == NULL)
			{
				//	出错了
				sts = STATUS_UNSUCCESSFUL;
				break;
			}

			PFSFILTER_WORKITEMCTX_CREATEDIR pWorkItemCtx = NULL;
			pWorkItemCtx = ExAllocatePoolWithTag(PagedPool,sizeof(FSFILTER_WORKITEMCTX_CREATEDIR), 'twic');
			if (pWorkItemCtx == NULL)
			{
				FltFreeDeferredIoWorkItem(pFltDeferWorkItem);
				sts = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			//	reparse到源volume
			if (wcsstr(Data->Iopb->TargetFileObject->FileName.Buffer, L"CopyOnClose.txt\0"))
			{
				UNICODE_STRING destpath = { 0 };
				sts = FsFltAllocUnicodeString((USHORT)1024, PagedPool, &destpath);
				if (!NT_SUCCESS(sts))
				{
					return sts;
				}

				RtlCopyUnicodeString(&destpath, &pEcpContext->OriginalFileName);

				sts = FileReparseOperation(Data, destpath, FltObjects);

				pEcpContext->IsReparseBack = TRUE;

				ReturnValue = FLT_POSTOP_FINISHED_PROCESSING;

				FsFilterFreeUnicodeString(&destpath);

				break;
			}

			pWorkItemCtx->pEcpCtx = pEcpContext;
			pWorkItemCtx->PreOrPost = WORKITEM_PREORPOST_POSTCALLBACK;
			sts = FltQueueDeferredIoWorkItem(pFltDeferWorkItem, Data, WorkItem_CreateDir, DelayedWorkQueue, pWorkItemCtx);
			if (!NT_SUCCESS(sts))
			{
				//	出错了
				FltFreeDeferredIoWorkItem(pFltDeferWorkItem);
				ExFreePoolWithTag(pWorkItemCtx, 'twic');
				break;
			}
			else
			{
				ReturnValue = FLT_POSTOP_MORE_PROCESSING_REQUIRED;
			}
		}
		else
		{
			if (NT_SUCCESS(Data->IoStatus.Status))
			{
				if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
				{
					KdPrint((__FUNCTION__"=> the file's parent directory should be opened, DesiredAccess : 0x%X\n",
						Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess));
				}

				FltIsDirectory(FltObjects->FileObject, FltObjects->Instance, &IsDir);
				if (IsDir)
				{
					/*KdPrint((__FUNCTION__"=> File : %wZ is directory, do not set file context!\n",
						FltObjects->FileObject->FileName));
					break;*/
				}

				//	Create成功
				//	设置FILE_CONTEXT参数
				sts = ApplFltCreateFileCtx(
					FltObjects->Instance,
					FltObjects->FileObject,//Data->Iopb->TargetFileObject,
					&pFileCtx);
				if (NT_SUCCESS(sts) && pFileCtx != NULL)
				{
					pFileCtx->m_bIsReparsed = TRUE;
					pFileCtx->pOriginalInstance = pEcpContext->pOriginalInstance;
					pFileCtx->m_bIsReparseBack = pEcpContext->IsReparseBack;
					RtlCopyMemory(&pFileCtx->OriginalFileName, &pEcpContext->OriginalFileName, sizeof(UNICODE_STRING));
					//FsFltCopyUnicodeString(&pFileCtx->OriginalFileName, &pEcpContext->OriginalFileName);
				}

				//	Todo : 还需要CreateOptions,Disposition等处理,例如：FILE_DELETE_ON_CLOSE,FILE_OVERWRITE
				if (FlagOn(CreateOption, FILE_DELETE_ON_CLOSE))
				{
					pFileCtx->m_bDelete = TRUE;
				}
			}

			//	如果是其它的错误status,有可能需要进一步处理
			//	此处暂且忽略，交给fltmgr
		}
	} while (0);

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	return ReturnValue;
}

NTSTATUS
FileReparseOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ UNICODE_STRING RedirectPath,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects
)
/*++

Routine Description:

	设置重解析数据 - 替换路径字串，设置status，添加ecp

Arguments:

	Data - 过滤驱动回调函数的call back 数据
	RedirectPath - 保存路径字串

Return Value:

	Routine can return non success error codes.
	STATUS_SUCCESS - 成功

--*/
{
	UNREFERENCED_PARAMETER(pFltObjects);

	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	if (NULL == RedirectPath.Buffer || 0 == RedirectPath.Length)
	{
		return STATUS_INVALID_PARAMETER;
	}
	/*
	if (RtlFindUnicodePrefix(&PrefixTable, &VirtualDesktopPath, 0))
	{
		KdBreakPoint();
	}
	*/
	//AddECP(Data, pFltObjects);

	rtn = IoReplaceFileObjectName(
		Data->Iopb->TargetFileObject,
		RedirectPath.Buffer,
		RedirectPath.Length);
	if (!NT_SUCCESS(rtn))
	{
		return rtn;
	}
	else
	{
		rtn = STATUS_SUCCESS;
		Data->IoStatus.Status = STATUS_REPARSE;
		Data->IoStatus.Information = IO_REPARSE;
	}

	return rtn;
}

NTSTATUS
FsFilterSetReparsePath(
	_In_ PFLT_CALLBACK_DATA Data,
	_Inout_ PUNICODE_STRING pRedirectPath,
	_In_ PFSFILTER_INSTANCE_CONTEXT pInstanceCtx
)
/*++

Routine Description:

	拼接新的路径字串

Arguments:

	Data - 过滤驱动回调函数的call back 数据
	RedirectPath - 保存路径字串
	pInstanceCtx - Instance的context指针

Return Value:

	Routine can return non success error codes.
	STATUS_SUCCESS - 成功

--*/
{
	UNREFERENCED_PARAMETER(Data);

	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	
	//	设置重定向参数
	//	重定向到DestVolume,\\Device\\HarddiskVolumeX
	sts = RtlAppendUnicodeStringToString(pRedirectPath, &pInstanceCtx->DestVolume);
	if (!NT_SUCCESS(sts))
	{
		return sts;
	}
	
	sts = RtlAppendUnicodeStringToString(pRedirectPath, &pInstanceCtx->DestPrefix);
	if (!NT_SUCCESS(sts))
	{
		return sts;
	}

	sts = RtlAppendUnicodeStringToString(pRedirectPath, &Data->Iopb->TargetFileObject->FileName);
	if (!NT_SUCCESS(sts))
	{
		return sts;
	}

	pRedirectPath->Buffer[pRedirectPath->Length] = 0;
	sts = STATUS_SUCCESS;

	return sts;
}

