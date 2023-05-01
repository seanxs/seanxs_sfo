#pragma once
#ifndef _SEANXSSFO_COMMON_H
#define _SEANXSSFO_COMMON_H

#include <fltKernel.h>
#include <dontuse.h>
#ifdef USING_TRACLOGGING
#include <TraceLoggingProvider.h>
#endif
#include <evntrace.h> // For TRACE_LEVEL definitions
#include <WppRecorder.h>

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
#define PTDBG_TRACE_REDIRECT_STATUS    0x00000004

#define SRC_PATH_NAME L"\\Device\\HarddiskVolume7\\seanxs_sfo_redirectout"
#define DEST_PATH_NAME L"\\Device\\HarddiskVolume10\\seanxs_sfo_redirectin"

#if DBG
#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))
#else
#define PT_DBG_PRINT( _dbgLevel, _string )
#endif

EXTERN_C_START

extern ULONG gTraceFlags;

VOID
seanxssfoOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
);

BOOLEAN
seanxssfoDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

EXTERN_C_END


#endif
