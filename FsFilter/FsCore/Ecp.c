/*++
Module Name:

	Ecp.c

Abstract:

	Extra create parameters (ECPs)需要的操作
	创建
	查找
	修改

Environment:
	Kernel Mode

--*/

#include "Ecp.h"
#include "..\FsCommon\Extern_def.h"

NPAGED_LOOKASIDE_LIST NPagedEcpLSL;

NTSTATUS InitEcpStaff()
/*++

Routine Description:

	初始化ecp所需资源
	NPAGED_LOOKASIDE_LIST
Arguments:

	无

Return Value:

	无

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	KdPrint((__FUNCTION__"=> enter\n"));

	FltInitExtraCreateParameterLookasideList(
		gFilterHandle,
		&NPagedEcpLSL,
		FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL,
		1024 * sizeof(FSFLT_REPARSE_INFO_ECP_CONTEXT),
		'ecpt');

	rtn = STATUS_SUCCESS;

	return rtn;
}

NTSTATUS CleanUpEcpStaff()
/*++

Routine Description:

	释放ecp所需资源
	NPAGED_LOOKASIDE_LIST
Arguments:

	无

Return Value:

	无

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	KdPrint((__FUNCTION__"=> enter\n"));

	FltDeleteExtraCreateParameterLookasideList(
		gFilterHandle,
		&NPagedEcpLSL,
		FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL);

	rtn = STATUS_SUCCESS;

	return rtn;
}

NTSTATUS AddECP(
	_In_ PFLT_CALLBACK_DATA pCBD,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Inout_ PFSFLT_REPARSE_INFO_ECP_CONTEXT *pEcpCtx)
/*++

Routine Description:

	添加ecp
Arguments:

	pCBD - IRP_MJ_CREATE操作的call back 数据
	pFltObjects - minifilter驱动中各种对象
	pEcpCtx - ecp上下文的指针地址

Return Value:

	STATUS_SUCCESS 或其它error code

--*/
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PECP_LIST ecpList = NULL;
	PFSFLT_REPARSE_INFO_ECP_CONTEXT pEcpContext = NULL;
	ULONG ECPContextSize = 0;

	PAGED_CODE();

	if (pCBD == NULL || pFltObjects == NULL || pEcpCtx == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		status = FltGetEcpListFromCallbackData(gFilterHandle, pCBD, &ecpList);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("FltGetEcpListFromCallbackData return : %08X\r\n", status));
			break;
		}

		if (ecpList == NULL)
		{
			//
			// Create a new ecplist.
			//
			status = FltAllocateExtraCreateParameterList(gFilterHandle, 0, &ecpList);
			if (!NT_SUCCESS(status))
			{
				KdPrint(("FltAllocateExtraCreateParameterList return : %08X\r\n", status));
				break;
			}

			//
			// Set it into CBD.
			//
			status = FltSetEcpListIntoCallbackData(gFilterHandle, pCBD, ecpList);
			if (!NT_SUCCESS(status)) {
				KdPrint(("FltSetEcpListIntoCallbackData return : %08X\r\n", status));
				break;
			}
		}
		else
		{
			status = FltFindExtraCreateParameter(
				gFilterHandle,
				ecpList,
				&GUID_FSFILTER_ECP_REPARSE_INFO,
				(PVOID*)&pEcpContext,
				&ECPContextSize);
			if (status == STATUS_SUCCESS)
			{
				if (NT_SUCCESS(status) && pEcpContext != NULL)
				{
					if (pEcpContext->IsReparsed)
					{
						*pEcpCtx = pEcpContext;
						status = STATUS_SUCCESS;
						break;
					}
				}
			}
			else if (status != STATUS_NOT_FOUND)
			{
				KdPrint(("FltFindExtraCreateParameter return : %08X\r\n", status));
				break;
			}
		}

		status = FltAllocateExtraCreateParameterFromLookasideList(
			gFilterHandle,
			&GUID_FSFILTER_ECP_REPARSE_INFO,
			sizeof(FSFLT_REPARSE_INFO_ECP_CONTEXT),
			FSRTL_ALLOCATE_ECP_FLAG_NONPAGED_POOL,
			NULL,
			&NPagedEcpLSL,
			(PVOID*)&pEcpContext);
		if (!NT_SUCCESS(status)) {
			KdPrint(("FltAllocateExtraCreateParameter return : %08X\r\n", status));
			break;
		}

		RtlZeroMemory(pEcpContext, sizeof(FSFLT_REPARSE_INFO_ECP_CONTEXT));
		pEcpContext->IsReparsed = TRUE;
		//pEcpContext->pOriginalInstance = pFltObjects->Instance;
		//pEcpContext->OrigProcId = PsGetCurrentProcessId();
		//pEcpContext->OrigThreadId = PsGetCurrentThreadId();
		*pEcpCtx = pEcpContext;

		status = FltInsertExtraCreateParameter(gFilterHandle, ecpList, pEcpContext);
		if (!NT_SUCCESS(status)) {
			KdPrint(("FltInsertExtraCreateParameter return : %08X\r\n", status));
			break;
		}

	} while (0);

	return status;
}

NTSTATUS GetECP(
	_Inout_ PFLT_CALLBACK_DATA pCBD,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Inout_ PFSFLT_REPARSE_INFO_ECP_CONTEXT *pEcpContext)
/*++

Routine Description:

	获取ecp
Arguments:

	pCBD - IRP_MJ_CREATE操作的call back 数据
	pFltObjects - minifilter驱动中各种对象
	pEcpCtx - ecp上下文的指针地址

Return Value:

	STATUS_SUCCESS 或其它error code

--*/
{
	UNREFERENCED_PARAMETER(pFltObjects);

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PECP_LIST ecpList = NULL;
	ULONG ECPContextSize = 0;

	if (pCBD == NULL || pEcpContext == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		*pEcpContext = NULL;
		status = FltGetEcpListFromCallbackData(gFilterHandle, pCBD, &ecpList);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("FltGetEcpListFromCallbackData return : %08X\r\n", status));
			break;
		}

		if (ecpList != NULL)
		{
			status = FltFindExtraCreateParameter(
				gFilterHandle,
				ecpList,
				&GUID_FSFILTER_ECP_REPARSE_INFO,
				(PVOID*)pEcpContext,
				&ECPContextSize);
			if (!NT_SUCCESS(status))
			{
				if (status != STATUS_NOT_FOUND)
				{
					KdPrint(("FltFindExtraCreateParameter return : %08X\r\n", status));
				}

				break;
			}

			//FltAcknowledgeEcp(gFilterHandle, pEcpContext);
		}
	} while (0);

	return status;
}
