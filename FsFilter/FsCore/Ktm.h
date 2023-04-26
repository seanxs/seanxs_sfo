#ifndef _FSFILTER_KTM_H
#define _FSFILTER_KTM_H

#include "..\FsCommon\Common.h"

NTSTATUS TransactionNtfCb(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_CONTEXT TransactionContext,
	_In_ ULONG NotificationMask
);

#endif // !_FSFILTER_KTM_H

