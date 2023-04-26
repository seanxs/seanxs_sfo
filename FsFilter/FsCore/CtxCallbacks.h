#pragma once
#ifndef _APPL_FSFILTER_CTXCALLBACK_H
#define _APPL_FSFILTER_CTXCALLBACK_H

#include "../FsCommon/Common.h"

#define FSFILTER_CONTEXT_TAG 'fsct'

typedef struct _FSFILTER_INSTANCE_CONTEXT
{
	PLIST_ENTRY pReparseDirList;
	PLIST_ENTRY pReparseFileList;
	UNICODE_STRING SrcDirPath;
	UNICODE_STRING DestDirPath;
	UNICODE_STRING DestPrefix;
	UNICODE_STRING DestVolume;
	UNICODE_STRING DestDirVolume;
	BOOLEAN RedirectOut;
	BOOLEAN EntireVolume;
	BOOLEAN RedirectIn;
}FSFILTER_INSTANCE_CONTEXT, *PFSFILTER_INSTANCE_CONTEXT;

typedef struct _FSFILTER_FILE_CONTEXT
{
	BOOLEAN m_bDelete;
	BOOLEAN m_bWrite;
	BOOLEAN m_bRename;
	BOOLEAN m_bIsReparsed;
	BOOLEAN m_bIsReparseBack;
	//FILE_INFORMATION_CLASS FileInfo;
	PFLT_INSTANCE pOriginalInstance;
	UNICODE_STRING OriginalFileName;
	//ULONG uHashValue;
}FSFILTER_FILE_CONTEXT, *PFSFILTER_FILE_CONTEXT;

typedef struct _FSFILTER_TRX_CONTEXT
{
	BOOLEAN m_bDelete;
	BOOLEAN m_bWrite;
	BOOLEAN m_bIsReparsed;
	//FILE_INFORMATION_CLASS FileInfo;
	PFLT_INSTANCE pOriginalInstance;
	UNICODE_STRING OriginalFileName;
	//ULONG uHashValue;
}FSFILTER_TRX_CONTEXT, *PFSFILTER_TRX_CONTEXT;

typedef struct _FSFILTER_STEAMHANDLE_CONTEXT
{
	BOOLEAN FirstQuery;
	FILE_INFORMATION_CLASS FileInfo;
}FSFILTER_STEAMHANDLE_CONTEXT, *PFSFILTER_STEAMHANDLE_CONTEXT;

EXTERN_C_START
extern const FLT_CONTEXT_REGISTRATION CtxReg[];

VOID FsFilterSteamHandleCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
);

VOID FsFilterInstanceCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
);

NTSTATUS FsFilterInstanceCtxCreate(
	_In_ PFLT_INSTANCE pInstance,
	_Out_ PFSFILTER_INSTANCE_CONTEXT * pContext
);

VOID FsFltFileCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
);

NTSTATUS ApplFltCreateFileCtx(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_Inout_ PFSFILTER_FILE_CONTEXT * pContext
);

NTSTATUS ApplFltGetFileCtx(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_Inout_ PFSFILTER_FILE_CONTEXT * pContext
);

VOID FsFltTrxCtxCleanup(
	_In_ PFLT_CONTEXT Context,
	_In_ FLT_CONTEXT_TYPE ContextType
);

EXTERN_C_END

#endif // !_APPL_FSFILTER_CTXCALLBACK_H
