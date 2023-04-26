#ifndef _FSFILTER_COMMUNICATION_H
#define _FSFILTER_COMMUNICATION_H

#include "../FsCommon/Common.h"

NTSTATUS EstablishCommunicate(
	_In_ PCWSTR pPortName,
	_In_ PFLT_FILTER pFilter);

VOID CloseCommunicate();

#endif // !_FSFILTER_COMMUNICATION_H