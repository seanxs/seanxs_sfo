/*++
Module Name:

	KernelObjs.c

Abstract:

	创建，使用，删除各种类型的kernel Object
	例如，list,workitem,mutex……等等

Environment:
	Kernel Mode

--*/

#include "KernelObjs.h"

NTSTATUS InitKernelObjs()
/*++

Routine Description:

	初始化kernel object,driverentry中调用

Arguments:

	无

Return Value:

	无

--*/
{
	NTSTATUS rtn = STATUS_SUCCESS;

	PAGED_CODE();

	KdPrint((__FUNCTION__"=>\n"));


	return rtn;
}

VOID CleanupKernelObjs()
/*++

Routine Description:

	清理kernel object，驱动退出前调用

Arguments:

	无

Return Value:

	无

--*/
{
	PAGED_CODE();

	KdPrint((__FUNCTION__"=>\n"));
	
	return;
}