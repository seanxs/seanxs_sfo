/*++
Module Name:

	nameprovider.c

Abstract:

	处理GenerateFileNameCallback请求
	基于minifilter技术

Environment:
	Kernel Mode

--*/

#include "nameprovider.h"
#include "FsCommon/Extern_def.h"

#include "FsCommon/Trace.h"
#include "nameprovider.tmh"

NTSTATUS FsFilterGeneFileName(
	_In_ PFLT_INSTANCE Instance,
	_In_ PFILE_OBJECT FileObject,
	_When_(FileObject->FsContext != NULL, _In_opt_)
	_When_(FileObject->FsContext == NULL, _In_)
	PFLT_CALLBACK_DATA Cbd,
	_In_ FLT_FILE_NAME_OPTIONS NameOptions,
	_Out_ PBOOLEAN CacheFileNameInformation,
	_Inout_ PFLT_NAME_CONTROL FileName
)
/*++

Routine Description:

	This routine generates a file name of the type specified in NameFormat
	for the specified file object.

Arguments:

	Instance - Opaque instance pointer for the minifilter driver instance that
	this callback routine is registered for.

	FileObject - The fileobject for which the name is being requested.

	Cbd - If non-NULL, the CallbackData structure defining the operation
		we are in the midst of processing when this name is queried.

	NameOptions - value that specifies the name format, query method, and flags
	for this file name information query

	CacheFileNameInformation - A pointer to a Boolean value specifying whether
	this name can be cached.

	FileName - A pointer to a filter manager-allocated FLT_NAME_CONTROL
	structure to receive the file name on output

Return Value:

	Returns STATUS_SUCCESS if a name could be returned, or the appropriate
	error otherwise.

--*/
{
	PFLT_FILE_NAME_INFORMATION userFileNameInfo = NULL;
	PUNICODE_STRING userFileName;
	NTSTATUS sts = STATUS_SUCCESS;

	do
	{

		//
		//  Clear FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER from the name options
		//  We pass the same name options when we issue a name query to satisfy this
		//  name query. We want that name query to be targeted below simrep.sys and
		//  not recurse into simrep.sys
		//

		ClearFlag(NameOptions, FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER);

		PAGED_CODE();

		if (FileObject->FsContext == NULL)
		{
			//
			//  This file object has not yet been opened.  We will query the filter
			//  manager for the name and return that name. We must use the original
			//  NameOptions we received in the query. If we were to swallow flags
			//  such as FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY or
			//  FLT_FILE_NAME_DO_NOT_CACHE we could corrupt the name cache.
			//

			sts = FltGetFileNameInformation(Cbd,
				NameOptions,
				&userFileNameInfo);

			if (!NT_SUCCESS(sts))
			{
				break;
			}

			userFileName = &userFileNameInfo->Name;

		}
		else
		{
			//
			//  The file has been opened. If the call is not in the context of an IO
			//  operation (we don't have a callback data), we have to get the
			//  filename with FltGetFilenameInformationUnsafe using the fileobject.
			//  Note, the only way we won't have a callback is if someone called
			//  FltGetFileNameInformationUnsafe already.
			//

			if (ARGUMENT_PRESENT(Cbd))
			{
				sts = FltGetFileNameInformation(Cbd,
					NameOptions,
					&userFileNameInfo);
			}
			else
			{
				sts = FltGetFileNameInformationUnsafe(FileObject,
					Instance,
					NameOptions,
					&userFileNameInfo);
			}

			if (!NT_SUCCESS(sts))
			{
				break;
			}

			userFileName = &userFileNameInfo->Name;

		}

		sts = FltCheckAndGrowNameControl(FileName, userFileName->Length);

		if (!NT_SUCCESS(sts))
		{
			break;
		}

		RtlCopyUnicodeString(&FileName->Name, userFileName);

		DoTraceLevelMessage(
			TRACE_LEVEL_INFORMATION,
			SEANXS_SFO_NAMEPROVIDER,
			"%!FILE!,%!FUNC!,%!LINE!=> File Object : %p , name : %wZ\n",
			FileObject,
			&FileName->Name);

		//
		//  If the file object is unopened then the name of the stream represented by
		//  the file object may change from pre-create to post-create.
		//  For example, the name being opened could actually be a symbolic link
		//

		*CacheFileNameInformation = (FileObject->FsContext != NULL);

	} while (0);

	if (userFileNameInfo != NULL)
	{
		FltReleaseFileNameInformation(userFileNameInfo);
	}

	return sts;
}

NTSTATUS
FsFilterNormalizeNameComponent(
	_In_ PFLT_INSTANCE Instance,
	_In_ PCUNICODE_STRING ParentDirectory,
	_In_ USHORT DeviceNameLength,
	_In_ PCUNICODE_STRING Component,
	_Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
	_In_ ULONG ExpandComponentNameLength,
	_In_ FLT_NORMALIZE_NAME_FLAGS Flags,
	_Inout_ PVOID *NormalizationContext
)
/*++

Routine Description:

	This routine normalizes, converts to a long name if needed, a name component.

Arguments:

	Instance - Opaque instance pointer for the minifilter driver instance that
	this callback routine is registered for.

	ParentDirectory - Pointer to a UNICODE_STRING structure that contains the
	name of the parent directory for this name component.

	VolumeNameLength - Length, in bytes, of the parent directory name that is
	stored in the structure that the ParentDirectory parameter points to.

	Component - Pointer to a UNICODE_STRING structure that contains the name
	component to be expanded.

	ExpandComponentName - Pointer to a FILE_NAMES_INFORMATION structure that
	receives the expanded (normalized) file name information for the name component.

	ExpandComponentNameLength - Length, in bytes, of the buffer that the
	ExpandComponentName parameter points to.

	Flags - Name normalization flags.

	NormalizationContext - Pointer to minifilter driver-provided context
	information to be passed in any subsequent calls to this callback routine
	that are made to normalize the remaining components in the same file name
	path.

Return Value:

	Returns STATUS_SUCCESS if a name could be returned, or the appropriate
	error otherwise.

--*/
{
	NTSTATUS status;
	HANDLE directoryHandle = NULL;
	PFILE_OBJECT directoryFileObject = NULL;
	OBJECT_ATTRIBUTES objAttributes;
	IO_STATUS_BLOCK ioStatusBlock;
	BOOLEAN ignoreCase = !BooleanFlagOn(Flags, FLTFL_NORMALIZE_NAME_CASE_SENSITIVE);

	UNREFERENCED_PARAMETER(NormalizationContext);
	UNREFERENCED_PARAMETER(DeviceNameLength);

	PAGED_CODE();

	//
	//  Validate the buffer is big enough
	//

	if (ExpandComponentNameLength < sizeof(FILE_NAMES_INFORMATION)) {

		return STATUS_INVALID_PARAMETER;
	}

	do
	{
		InitializeObjectAttributes(
			&objAttributes,
			(PUNICODE_STRING)ParentDirectory,
			OBJ_KERNEL_HANDLE
			| (ignoreCase ? OBJ_CASE_INSENSITIVE : 0),
			NULL,
			NULL);

		status = FltCreateFile(
			gFilterHandle,
			Instance,
			&directoryHandle,
			FILE_LIST_DIRECTORY | SYNCHRONIZE, // DesiredAccess
			&objAttributes,
			&ioStatusBlock,
			NULL,                              // AllocationSize
			FILE_ATTRIBUTE_DIRECTORY
			| FILE_ATTRIBUTE_NORMAL,         // FileAttributes
			FILE_SHARE_READ
			| FILE_SHARE_WRITE
			| FILE_SHARE_DELETE,             // ShareAccess
			FILE_OPEN,                         // CreateDisposition
			FILE_DIRECTORY_FILE
			| FILE_SYNCHRONOUS_IO_NONALERT
			| FILE_OPEN_FOR_BACKUP_INTENT,   // CreateOptions
			NULL,                              // EaBuffer
			0,                                 // EaLength
			IO_IGNORE_SHARE_ACCESS_CHECK);    // Flags

		if (!NT_SUCCESS(status))
		{
			break;
		}

		status = ObReferenceObjectByHandle(
			directoryHandle,
			FILE_LIST_DIRECTORY | SYNCHRONIZE, // DesiredAccess
			*IoFileObjectType,
			KernelMode,
			&directoryFileObject,
			NULL);

		if (!NT_SUCCESS(status))
		{
			break;
		}

		//
		//  Query the file entry to get the long name
		//

		status = FltQueryDirectoryFile(
			Instance,
			directoryFileObject,
			ExpandComponentName,
			ExpandComponentNameLength,
			FileNamesInformation,
			TRUE, /* ReturnSingleEntry */
			(PUNICODE_STRING)Component,
			TRUE, /* restartScan */
			NULL);

	} while (0);

	if (NULL != directoryHandle) {

		FltClose(directoryHandle);
	}

	if (NULL != directoryFileObject) {

		ObDereferenceObject(directoryFileObject);
	}

	return status;

}