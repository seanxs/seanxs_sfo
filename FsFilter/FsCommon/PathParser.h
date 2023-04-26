#pragma once

#include "Common.h"
VOID InitVolPathInfoResource();
VOID ReleaseVolPathInfoResource();
NTSTATUS ScanAllVolume();

NTSTATUS
RedirectCheck(
	IN PUNICODE_STRING pusSrcPath,
	IN UCHAR           bRWFlag,
	IN UCHAR           bCreateFlag,
	OUT BOOLEAN* pbNeedRedirect,
	OUT PUNICODE_STRING pusRedirectPath
);