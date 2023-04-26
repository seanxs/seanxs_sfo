#include "CommonFuncs.h"

NTSTATUS FsFilterFreeUnicodeString(
	_Inout_ PUNICODE_STRING pString
)
/*++

Routine Description:

	�ͷ�Unicode�ִ����ڴ棬����pString����
	���ͷŵ�Unicode�ִ�����Ҫ��ͨ��FsFltAllocUnicodeString�������

Arguments:

	pString - Unicode�ִ�������ַ

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

	Ϊһ���յ�Unicode�ִ������ڴ�

Arguments:

	Size - Unicode�ִ���buffer�Ĵ�С�����ֽ�Ϊ��λ
	pool - Unicode�ִ���buffer���ڴ����ͣ�PagedPool �� NonPagedPool
	pString - Unicode�ִ�������ַ

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

		��ý�������ļ�����Ϣ
		�������ͷ�pNameInfo

	Arguments:

		pCbd - ���������ص�������call back ����
		pNameInfo - �����ļ�����Ϣ��buffer

	Return Value:

		Routine can return non success error codes.
		STATUS_SUCCESS - �ɹ�

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
