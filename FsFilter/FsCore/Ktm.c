/*++
Module Name:

	Ktm.c

Abstract:

	Kernel Transaction Manager相关的操作

Environment:
	Kernel Mode

--*/

#include "Ktm.h"
#include "CtxCallbacks.h"


NTSTATUS TransactionNtfCb(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_CONTEXT TransactionContext,
	_In_ ULONG NotificationMask
)
/*++

Routine Description:

	The registered routine of type PFLT_TRANSACTION_NOTIFICATION_CALLBACK
	in FLT_REGISTRATION structure.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	TransactionContext - Pointer to the minifilter driver's transaction context
		set at PostCreate.

	TransactionNotification - Specifies the type of notifications that the
		filter manager is sending to the minifilter driver.

Return Value:

	STATUS_SUCCESS - Returning this status value indicates that the minifilter
		driver is finished with the transaction. This is a success code.

--*/
{
	UNREFERENCED_PARAMETER(NotificationMask);
	UNREFERENCED_PARAMETER(TransactionContext);

	NTSTATUS sts = STATUS_UNSUCCESSFUL;
	PFSFILTER_FILE_CONTEXT pFileCtx = NULL;

	do
	{
		sts = FltGetFileContext(
			FltObjects->Instance,
			FltObjects->FileObject,
			&pFileCtx);
		if (!NT_SUCCESS(sts))
		{
			if (sts == STATUS_NOT_FOUND)
			{
				//	没有file context，说明没有reparse
				sts = STATUS_SUCCESS;
				break;

			}
			else
			{
				//	有错误发生
				break;
			}

		}
	} while (0);

	if (!NT_SUCCESS(sts))
	{
		KdPrint((__FUNCTION__"=> error code : 0x%08X\n", sts));
	}

	if (pFileCtx)
	{
		FltReleaseContext(pFileCtx);
	}

	return sts;
}