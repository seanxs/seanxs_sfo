/*++
Module Name:

	Communication.c

Abstract:

	MiniFilter驱动与用户层Agent交互

Environment:
	Kernel Mode

--*/

#include "Communication.h"

const PWSTR AppLFsFilterPortName = L"\\AppLFsFilterPort";
PFLT_PORT g_FsFilterSrvPort = NULL;

NTSTATUS
EstablishCommunicate(
	_In_ PCWSTR pPortName,
	_In_ PFLT_FILTER pFilter)
/*++

Routine Description:

	创建通讯端口，用来与用户态程序交互
	需要在过滤驱动注册成功后，再创建

Arguments:

	pPortName - 端口名称

Return Value:

	STATUS_SUCCESS - 与通讯端口建立成功
	其他值 - 相应的错误代码

--*/
{
	//NTSTATUS sts = STATUS_UNSUCCESSFUL;
	NTSTATUS sts = STATUS_SUCCESS;

	PAGED_CODE();

	KdPrint((__FUNCTION__"=>\n"));

	if (pPortName == NULL || pFilter == NULL)
	{
		sts = STATUS_INVALID_PARAMETER;
	}

	return sts;
}

VOID CloseCommunicate()
/*++

Routine Description:

	关闭通讯端口
	驱动结束前调用

Arguments:

	无

Return Value:

	无

--*/
{
	PAGED_CODE();

	KdPrint((__FUNCTION__"=>\n"));

	if (NULL != g_FsFilterSrvPort)
	{
		FltCloseCommunicationPort(g_FsFilterSrvPort);
	}

	g_FsFilterSrvPort = NULL;

	return;
}