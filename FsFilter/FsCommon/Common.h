#pragma once

#include <fltKernel.h>
#include <dontuse.h>
#include <Ntstrsafe.h>

#define SUPPORT_UNLOAD_DRIVER

#ifndef  INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (void*)-1
#endif // ! INVALID_HANDLE_VALUE

typedef struct _APPL_REPARSE_PAIR_INFO
{
	LIST_ENTRY ListEntry;
	PFLT_INSTANCE DestInstance;
	ULONG SrcHash;
	ULONG DestHash;
	UNICODE_STRING SrcFullPath;
	UNICODE_STRING DestFullPath;
}
APPL_REPARSE_PAIR_INFO, * PAPPL_REPARSE_PAIR_INFO;

// Table data is returned at the end of the SYSTEM_FIRMWARE_INFORMATION.
typedef struct _SYSTEM_FIRMWARE_INFORMATION {
	ULONG FirmwareTableProviderSignature;
	ULONG Unknown;
	ULONG FirmwareTableID;
	ULONG FirmwareTableSize;
	UCHAR Data[1];
} SYSTEM_FIRMWARE_INFORMATION, * PSYSTEM_FIRMWARE_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemHandleInformation = 16,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45,
	SystemFirmwareTableInformation = 76,
	SystemFirmwareInformation = 0x4c,
	SystemPolicyInformation = 134,
} SYSTEM_INFORMATION_CLASS;


//typedef struct _APPL_WORK_MODE_INFO {
//	VD_FILEFILTER_MODE gAPPLWorkMode;
//	BOOLEAN bOpenAppLayerMode;
//	APPL_PERSONAL_MODE_INFO PersonalModeInfo;
//	BOOLEAN bOpenUPMMode;
//	BOOLEAN bOpenUAMMode;
//}APPL_WORK_MODE_INFO, * PAPPL_WORK_MODE_INFO;

//typedef enum _APPL_WORK_MODE {
//	APPL_WORK_MODE_UNKNOWN = 0,
//	APPL_WORK_MODE_AppLayer,
//	APPL_WORK_MODE_PersonalCofig,
//	APPL_WORK_MODE_PersonalUse,
//	APPL_WORK_MODE_UPM,
//	APPL_WORK_MODE_UAM,
//	APPL_WORK_MODE_UPM_UAM,
//	APPL_WORK_MODE_AppLayer_UAM,
//
//	APPL_WORK_MODE_Max = 100
//}APPL_WORK_MODE;

typedef struct _FILTER_PATH_STRUCT {

	LIST_ENTRY	   ListEntry;

	ULONG			uHash;

	USHORT			Length;

	USHORT			MaxLength;

	PWCH			Buffer;

}FILTER_PATH_STRUCT, * PFILTER_PATH_STRUCT;

typedef struct _FILTER_PATH_HASH_STRUCT {

	LIST_ENTRY	   ListEntry;

	ULONG			uPathHashKey;

}FILTER_PATH_HASH_STRUCT, * PFILTER_PATH_HASH_STRUCT;
