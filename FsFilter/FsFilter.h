#ifndef _FSFILTER_H
#define _FSFILTER_H

#include "FsCommon/Common.h"

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
FsFilterUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);
#endif