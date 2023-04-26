#pragma once
#ifndef _APPL_FSFILTER_ECP_H
#define _APPL_FSFILTER_ECP_H

#include "..\FsCommon\Common.h"

// {94BF3FE7-65F8-4E0C-892F-72548E998E78}
DEFINE_GUID(GUID_FSFILTER_ECP_REPARSE_INFO,
	0x94bf3fe7, 0x65f8, 0x4e0c, 0x89, 0x2f, 0x72, 0x54, 0x8e, 0x99, 0x8e, 0x78);

EXTERN_C_START

typedef struct _FSFLT_REPARSE_INFO_ECP_CONTEXT
{
	PFLT_INSTANCE pOriginalInstance;
	PFLT_VOLUME pOriginalVolume;
	UNICODE_STRING OriginalFileName;	//	buffer����FsFltFileCtxCleanup�����ﱻ�ͷ�
	BOOLEAN IsReparsed;
	BOOLEAN IsReparseBack;
}
FSFLT_REPARSE_INFO_ECP_CONTEXT, *PFSFLT_REPARSE_INFO_ECP_CONTEXT;

NTSTATUS AddECP(
	_In_ PFLT_CALLBACK_DATA pCBD,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Inout_ PFSFLT_REPARSE_INFO_ECP_CONTEXT *pEcpCtx);

NTSTATUS GetECP(
	_Inout_ PFLT_CALLBACK_DATA pCBD,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Inout_ PFSFLT_REPARSE_INFO_ECP_CONTEXT *pEcpContext);

NTSTATUS InitEcpStaff();
NTSTATUS CleanUpEcpStaff();

EXTERN_C_END

#endif // !_APPL_FSFILTER_ECP_H