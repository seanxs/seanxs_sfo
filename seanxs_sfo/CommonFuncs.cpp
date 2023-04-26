#include "CommonFuncs.h"

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

	if (pString == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pString->Buffer != NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	pString->Buffer = (PWCH)ExAllocatePoolWithTag(pool, Size, UNISTR_TAG);
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
			FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
			pNameInfo);
	}

	if (NT_SUCCESS(rtn))
	{
		rtn = FltParseFileNameInformation(*pNameInfo);
	}

	return rtn;
}
