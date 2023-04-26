#include "Trace.h"

NTSTATUS RegEtw(OUT PREGHANDLE phEtw)
{
	PAGED_CODE();

	NTSTATUS sts = STATUS_SUCCESS;
	if (!phEtw)
	{
		return STATUS_INVALID_PARAMETER;
	}

	sts = EtwRegister(&seanxs_sfo_etw, NULL, NULL, phEtw);

	return sts;
}

NTSTATUS UnRegEtw(IN REGHANDLE hEtw)
{
	PAGED_CODE();

	NTSTATUS sts = STATUS_SUCCESS;

	if (!hEtw)
	{
		return STATUS_INVALID_PARAMETER;
	}

	sts = EtwUnregister(hEtw);
	
	return sts;
}