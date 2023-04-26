/*++
Module Name:

	CommonFunctions.c

Abstract:

	文件过滤驱动的公用函数模块

Environment:
	Kernel Mode

--*/

#include "Extern_def.h"
#include "CommonFunctions.h"

NTSTATUS FsFltCopyUnicodeString(
	_In_ PUNICODE_STRING pDstUniStr,
	_In_ PUNICODE_STRING pSrcUniStr
)
/*++

Routine Description:

	复制unicode字符串

Arguments:

	pDstUniStr - 目标Unicode字串变量地址
	pSrcUniStr - 源Unicode字串变量地址

Return Value:

	Routine can return non success error codes.

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	if (pDstUniStr == NULL || pSrcUniStr == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pSrcUniStr->Buffer == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pDstUniStr->Buffer == NULL)
	{
		rtn = FsFltAllocUnicodeString(pSrcUniStr->Length, NonPagedPool, pDstUniStr);
		if (!NT_SUCCESS(rtn))
		{
			return rtn;
		}
	}

	if (pDstUniStr->MaximumLength < pSrcUniStr->Length)
	{
		return STATUS_INVALID_PARAMETER;
	}

	RtlCopyUnicodeString(pDstUniStr, pSrcUniStr);

	return STATUS_SUCCESS;
}

NTSTATUS FsFilterFreeUnicodeString(
	_Inout_ PUNICODE_STRING pString
)
/*++

Routine Description:

	释放Unicode字串的内存，并将pString清零
	被释放的Unicode字串，需要是通过FsFltAllocUnicodeString分配而来

Arguments:

	pString - Unicode字串变量地址

Return Value:

	Routine can return non success error codes.

--*/
{
	if (pString == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pString->Buffer == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	ExFreePoolWithTag(pString->Buffer, UNISTR_TAG);
	RtlZeroMemory(pString, sizeof(UNICODE_STRING));

	return STATUS_SUCCESS;
}
NTSTATUS FsFltAllocUnicodeString(
	_In_ USHORT Size,
	_In_ POOL_TYPE pool,
	_Inout_ PUNICODE_STRING pString
)
/*++

Routine Description:

	为一个空的Unicode字串分配内存

Arguments:

	Size - Unicode字串里buffer的大小，以字节为单位
	pool - Unicode字串里buffer的内存类型，PagedPool 或 NonPagedPool
	pString - Unicode字串变量地址

Return Value:

	Routine can return non success error codes.

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	if (Size == 0)
	{
		return STATUS_INVALID_PARAMETER;
	}

	/*if (!(pool == PagedPool || pool == NonPagedPool))
	{
		return STATUS_INVALID_PARAMETER;
	}*/

	if (pString == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pString->Buffer != NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	pString->Buffer = ExAllocatePoolWithTag(pool, Size, UNISTR_TAG);
	if (pString->Buffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		RtlZeroMemory(pString->Buffer, Size);

		pString->Length = 0;
		pString->MaximumLength = Size;

		rtn = STATUS_SUCCESS;
	}

	return rtn;
}

NTSTATUS ApplFsOpenFile(
	_In_ PUNICODE_STRING pFileName,
	_In_ PFLT_INSTANCE pInstance,
	_Inout_ PHANDLE phFile,
	_Inout_ PFILE_OBJECT *pFileObj
)
/*++

Routine Description:

	minifilter 环境下打开文件

Arguments:

	pFileName - 文件名
	pInstance - Instance句柄
	pFileObj - 成功打开后的文件对象
	phFile - 成功打开后的文件句柄

Return Value:

	Routine can return non success error codes.
	STATUS_SUCCESS - 打开成功

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	if (pFileName == NULL || phFile == NULL || pInstance == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	InitializeObjectAttributes(
		&ObjectAttributes,
		pFileName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL);

	rtn = FltCreateFileEx2(
		gFilterHandle,	//	Filter
		pInstance,	//	Instance
		phFile,	//	FileHandle
		pFileObj,	//	FileObject
		GENERIC_READ,	//	DesiredAccess
		&ObjectAttributes,	//	ObjectAttributes
		&IoStatusBlock,	//	IoStatusBlock
		NULL,	//	AllocationSize
		FILE_ATTRIBUTE_NORMAL,	//	FileAttributes
		FILE_SHARE_READ | FILE_SHARE_WRITE,	//	ShareAccess
		FILE_OPEN, //	CreateDisposition
		FILE_SYNCHRONOUS_IO_NONALERT, //FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,	//	CreateOptions
		NULL,	//	EaBuffer
		0,	//	EaLength
		0,	//	Flags
		NULL);	//	DriverContext

	return rtn;
}

NTSTATUS ApplFsGetFileSize(
	_In_ PFILE_OBJECT pFileObj,
	_In_ PFLT_INSTANCE pInstance,
	_Inout_ LARGE_INTEGER *pSize
)
/*++

Routine Description:

	获取文件大小

Arguments:

	pFileObj - 文件对象
	pInstance - Instance句柄
	pSize - 文件大小

Return Value:

	Routine can return non success error codes.
	STATUS_SUCCESS - 打开成功

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;
	FILE_STANDARD_INFORMATION FileStdInfo = { 0 };

	PAGED_CODE();
	FLT_ASSERT(IoGetTopLevelIrp() == NULL);

	if (pFileObj == NULL || pInstance == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pSize)
	{
		rtn = FltQueryInformationFile(
			pInstance,
			pFileObj,
			&FileStdInfo,
			sizeof(FileStdInfo),
			FileStandardInformation,
			NULL);
		if (NT_SUCCESS(rtn))
		{
			*pSize = FileStdInfo.EndOfFile;
		}
	}
	else
	{
		rtn = STATUS_INVALID_PARAMETER;
	}

	return rtn;
}

NTSTATUS GetParsedNameInfo(
	_In_ PFLT_CALLBACK_DATA pCbd,
	_Inout_ PFLT_FILE_NAME_INFORMATION *pNameInfo)
/*++

Routine Description:

	获得解析后的文件名信息
	调用者释放pNameInfo

Arguments:

	pCbd - 过滤驱动回调函数的call back 数据
	pNameInfo - 保存文件名信息的buffer

Return Value:

	Routine can return non success error codes.
	STATUS_SUCCESS - 成功

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	if (pCbd == NULL || pNameInfo == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (FlagOn(pCbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
	{

		//
		//  The SL_OPEN_TARGET_DIRECTORY flag indicates the caller is attempting
		//  to open the target of a rename or hard link creation operation. We
		//  must clear this flag when asking fltmgr for the name or the result
		//  will not include the final component. We need the full path in order
		//  to compare the name to our mapping.
		//

		ClearFlag(pCbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);

		rtn = FltGetFileNameInformation(
			pCbd,
			FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY,
			pNameInfo);

		//
		//  Restore the SL_OPEN_TARGET_DIRECTORY flag so the create will proceed
		//  for the target. The file systems depend on this flag being set in
		//  the target create in order for the subsequent SET_INFORMATION
		//  operation to proceed correctly.
		//

		SetFlag(pCbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);
	}
	else
	{
		//
		//  Note that we use FLT_FILE_NAME_QUERY_DEFAULT when querying the
		//  filename. In the precreate the filename should not be in filter
		//  manager's name cache so there is no point looking there.
		//

		rtn = FltGetFileNameInformation(
			pCbd,
			FLT_FILE_NAME_OPENED |	FLT_FILE_NAME_QUERY_DEFAULT,
			pNameInfo);
	}

	if (NT_SUCCESS(rtn))
	{
		rtn = FltParseFileNameInformation(*pNameInfo);
	}

	return rtn;
}

NTSTATUS FsFltReadFile(
	_In_ PFILE_OBJECT pFileObj,
	_In_ PFLT_INSTANCE pInstance,
	_In_ PVOID pBuf,
	_In_ ULONG Size,
	_In_ PLARGE_INTEGER Offset,
	_Inout_ ULONG_PTR *pReadLen
)
/*++

Routine Description:

	从文件中读数据

Arguments:

	pFileName - 文件名
	pInstance - Instance句柄
	phFile - 成功打开后的文件句柄

Return Value:

	Routine can return non success error codes.
	STATUS_SUCCESS - 打开成功

--*/
{
	NTSTATUS rtn = STATUS_UNSUCCESSFUL;

	if (pFileObj == NULL ||
		pInstance == NULL ||
		pBuf == NULL ||
		Size == 0)
	{
		return STATUS_INVALID_PARAMETER;
	}

	UNREFERENCED_PARAMETER(Offset);
	
	*pReadLen = 0;

	return rtn;
}

NTSTATUS
VDFileFilterAllocateUnicodeString(
	_Inout_ PUNICODE_STRING String,
	_In_	ULONG	uTag
)
{
	PAGED_CODE();
	String->Buffer = ExAllocatePoolWithTag(NonPagedPool,
		String->MaximumLength,
		uTag);

	if (String->Buffer == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(String->Buffer, String->MaximumLength);

	String->Length = 0;

	return STATUS_SUCCESS;
}

VOID
VDFileFilterFreeUnicodeString(
	_Inout_ PUNICODE_STRING String,
	_In_	ULONG	uTag
)
/*++

Routine Description:

This routine frees a unicode string

Arguments:

String - supplies the string to be freed

Return Value:

None

--*/
{
	PAGED_CODE();

	if (String->Buffer) {

		ExFreePoolWithTag(String->Buffer,
			uTag);
		String->Buffer = NULL;
	}

	String->Length = String->MaximumLength = 0;
	String->Buffer = NULL;
}
