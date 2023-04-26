#include "PathParser.h"
#include "GetConfig.h"
#include "..\..\Tools\Log\RJLog.h"

extern LIST_ENTRY		g_ExcludeFileList; //保存文件例外名单,文件例外名单链表里保存的是Hash值
extern LIST_ENTRY		g_ExcludeDirList ; //保存目录例外名单，目录例外名单链表里保存的是字串

#define MAX_VOLUME_NO  999
#define MAX_VALID_VOLUME_COUNT 20

#define MAX_VOLUME_PATH_LEN sizeof(L"\\Device\\HarddiskVolumexxx\0")
#define PATH_PARSER_MEM_TAG  'PPME'

typedef struct _VOL_PATH_INFO {
	UNICODE_STRING gusVolumePath[1];
	int            nVolCount;
}VOL_PATH_INFO , *PVOL_PATH_INFO;

VOL_PATH_INFO gVolPathInfo;

VOID InitVolPathInfoResource()
{
	gVolPathInfo.nVolCount = 0;
}

VOID ReleaseVolPathInfoResource()
{
	for (int i = 0; i < gVolPathInfo.nVolCount; i++)
	{
		if (NULL != gVolPathInfo.gusVolumePath[i].Buffer)
		{
			ExFreePoolWithTag(gVolPathInfo.gusVolumePath[i].Buffer, PATH_PARSER_MEM_TAG);
			gVolPathInfo.gusVolumePath[i].Buffer = NULL;
		}
	}
}

NTSTATUS ScanAllVolume()
/*++

Routine Description:

	扫描当前系统中的所有卷名称信息（\Device\HarddiskVolumeX），保存到全局数组中

Arguments:

	None

Return Value:

	STATUS_SUCCESS - 扫描成功
	其他值 - 相应的错误代码。

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES					ObjAttr = { 0 };
	IO_STATUS_BLOCK						IoStatusBlock = { 0 };
	HANDLE								hSymbolic = NULL;
	UNICODE_STRING                      uVolumePath = { 0 };
	 
	RjPrint("enter ScanAllVolume");

	uVolumePath.Length = 0;
	uVolumePath.MaximumLength = MAX_VOLUME_PATH_LEN;
	uVolumePath.Buffer = ExAllocatePoolWithTag(NonPagedPoolNx, uVolumePath.MaximumLength, PATH_PARSER_MEM_TAG);
	if (uVolumePath.Buffer == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	gVolPathInfo.nVolCount = 0;
	for (int i = 0, j = 0; i < MAX_VOLUME_NO; i++)
	{
		RtlZeroMemory(uVolumePath.Buffer, uVolumePath.MaximumLength);
		RtlUnicodeStringPrintf(&uVolumePath, L"\\Device\\HarddiskVolume%d",i);

		InitializeObjectAttributes(&ObjAttr,
			&uVolumePath,
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);

		status = ZwCreateFile(&hSymbolic,
			GENERIC_READ,
			&ObjAttr,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_VALID_FLAGS,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE,
			NULL, 0);
		if (NT_SUCCESS(status))
		{
			//RjPrint("%wZ is exist!", &uVolumePath);
			gVolPathInfo.gusVolumePath[j].Length = 0;
			gVolPathInfo.gusVolumePath[j].MaximumLength = MAX_VOLUME_PATH_LEN;
			gVolPathInfo.gusVolumePath[j].Buffer = 
				ExAllocatePoolWithTag(
					NonPagedPoolNx,
					gVolPathInfo.gusVolumePath[j].MaximumLength, 
					PATH_PARSER_MEM_TAG);
			if (gVolPathInfo.gusVolumePath[j].Buffer == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}

			RtlCopyUnicodeString(&gVolPathInfo.gusVolumePath[j], &uVolumePath);
			RjPrint("gVolPathInfo.gusVolumePath[%ld]: %wZ is exist!",j, &gVolPathInfo.gusVolumePath[j]);

			j += 1;
			gVolPathInfo.nVolCount = j;
			ZwClose(hSymbolic);
		}
	}

cleanup:
	if (NULL != uVolumePath.Buffer)
	{
		ExFreePoolWithTag(uVolumePath.Buffer, PATH_PARSER_MEM_TAG);
		uVolumePath.Buffer = NULL;
	}

	return status;

}

NTSTATUS 
RedirectCheck( 
	IN PUNICODE_STRING pusSrcPath,
	IN UCHAR           bRWFlag,
	IN UCHAR           bCreateFlag,
	OUT BOOLEAN*       pbNeedRedirect,
	OUT PUNICODE_STRING pusRedirectPath
)
/*++

Routine Description:

	检测传入的SrcPath是否需要重定向

Arguments:

	pusSrcPath      - 待检测是否需要重定向的源路径（需要是全路径）,盘符需要是\Device\HarddiskVolumeX这样的
	                  eg: \Device\HarddiskVolume3\Windows\System32\ucrtbase.dll
	bRWFlag         - 读写flag
	bCreateFlag     - 标记是否是Create操作
	pbNeedRedirect  - 返回是否需要重定向
	pusRedirectPath - 如果需要重定向返回重定向的路径前缀

Return Value:
    
	STATUS_SUCCESS - 检测函数里面没有遇到错误
	其他值 - 相应的错误代码。调用者需要在改函数返回STATUS_SUCCESS情况下才能使用传出参数

--*/
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pusSrcPath);
	UNREFERENCED_PARAMETER(bRWFlag);
	UNREFERENCED_PARAMETER(bCreateFlag);
	//UNREFERENCED_PARAMETER(pbNeedRedirect);
	UNREFERENCED_PARAMETER(pusRedirectPath);

	UNICODE_STRING							uniLocalTemp = { 0 };
	PUNICODE_STRING							pUniFilePath = NULL;
	BOOLEAN                                 bIsDir = FALSE;

	if ((NULL == pusSrcPath) || (NULL == pbNeedRedirect) || (NULL == pusRedirectPath))
	{
		status = STATUS_INVALID_PARAMETER;
		goto RET;
	}

	*pbNeedRedirect = FALSE;

	uniLocalTemp.Length = pusSrcPath->Length;
	uniLocalTemp.MaximumLength = pusSrcPath->MaximumLength;
	uniLocalTemp.Buffer = pusSrcPath->Buffer;

	USHORT	uLength = 0;
	uLength = uniLocalTemp.Length / sizeof(WCHAR);

	//如果文件夹路径最后一个是斜杠，说明当前访问的源路径是目录,要区分是不是目录是因为例外名单里文件和目录是分开存储的
	//在检测时需要在不同的链表里去查询
	if (uniLocalTemp.Buffer[uLength - 1] == L'\\')
	{
		//RjPrint("pusSrcPath : %wZ", pusSrcPath);
		uniLocalTemp.Length = uniLocalTemp.Length - sizeof(WCHAR);
		uLength -= 1;
		bIsDir = TRUE;
	}

	pUniFilePath = &uniLocalTemp;

	//Step1: 例外名单检测，如果在例外名单里则不需要重定向

	if (bIsDir)//按目录来进行检测
	{
		if (TRUE == JudgeThePathInFilterList(pusSrcPath, &g_ExcludeDirList))
		{
			*pbNeedRedirect = TRUE;
			//RjPrint("zhxunCC %wZ is in exclude dir list", pusSrcPath);
			goto RET;
		}
	}
	else //按文件来进行检测
	{
		if (TRUE == JudgeThePathHashEqualInFilterList(pusSrcPath, &g_ExcludeFileList))
		{
			*pbNeedRedirect = TRUE;
			//RjPrint("zhxunCC %wZ is in exclude file list", pusSrcPath);
			goto RET;
		}
	}
	
	//Step2:缓存检测，如果在缓存里有记录，则直接取出来使用，不需要再通过其他方式进行检测


	//需要通过最原始方法来进行检测

	//首先根据源路径转换得到当前场景下的重定向路径

	//检测重定向路径是否存在，如果存在则需要重定向，返回


	//检测源路径文件是否存在，如果存在则返回源路径，不需要重定向，返回源路径

	//源路径和重定向路径都不存在时，考虑其他特殊情况检测

	//创建操作

	//打开操作


/*
	if (原始路径在例外名单里）
		{
			返回原始路径；
			return；
		}

		if (在缓存hashtable里找到原始路径记录)
		{
			取出记录里的重定向位置前缀编码；
				解析出来重定向路径；
				return；
		}

	//路径转换，得到原始路径对应的重定向路径
	switch (应用分层场景)
	{
		case 应用分层桌面：
		{
			将原始路径盘符替换成应用分层桌面重定向盘得到重定向路径；
		}
		break;
		case 个性化配置模式：//有问题待重新思考
		{
			将原始路径盘符替换成个性化配置模式重定向盘得到重定向路径；
		}
		break;
		case 个性化使用模式：
		{
			将原始路径盘符替换成个性化使用模式重定向盘得到重定向路径；
		}
		break;
		case UPM模式：
		{
			将原始路径盘符替换成UPM模式重定向盘 + UUID目录得到重定向路径；
		}
		break;
		case UAM模式：
		{
		   for (int i = 0; i < UAM盘个数；i++)
		   {
			  if (写)
			  {
				  将原始路径盘符替换成该UAM模式重定向完整目录得到重定向路径；
			  }
			  else if (读)
			  {
				  将原始路径盘符替换成该UAM模式重定向盘得到重定向路径；
			  }
		   }
		}
		break;
		case UAM + UPM模式：
		{
			将原始路径盘符替换成UPM模式重定向盘 + UUID目录得到重定向路径；
			for (int i = 0; i < UAM盘个数；i++)
		   {
			  if (写)
			  {
				  将原始路径盘符替换成该UAM模式重定向完整目录得到重定向路径；
			  }
			  else if (读)
			  {
				  将原始路径盘符替换成该UAM模式重定向盘得到重定向路径；
			  }
		   }
		}
		break;
	}

	//检测重定向路径文件是否存在
	if（有UPM模式存在，先检测UPM路径）
	{
	   if (源路径在UPM配置列表里）
	   {
		   返回该重定向路径；
		   添加源路径 + 重定向路径盘符编码到hash table里；
		   return；
	   }
		   }

		   if (通过createfile打开远端文件成功)
		   {
			   返回该重定向路径；
				   添加源路径 + 重定向路径盘符编码到hash table里；
				   return；
		   }

	//检测原始路径文件是否存在
	if (通过createfile打开原始路径文件成功)
	{
		返回该原始文件路径；
			添加源路径 + 源路径盘符编码到hash table里；
			return；
	}

	//原始路径文件和重定向路径文件都不存在时 
	if (创建操作)
	{
		//正在思考处理流程
		return；
	}

	if (打开）
	{
		//在需要重定向情况下，如果远端没有就打开本地，如果远端有打开
			return；
	}
*/
RET:
	return status;
}