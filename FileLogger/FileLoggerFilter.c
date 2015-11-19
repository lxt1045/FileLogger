#include "FileLoggerFilter.h"
#include "StringHashMap.h"
#include "Container.h"
#include "FileLoggerData.h"
#include "HashFunction.h"

#define FILTER_KEY_NAME L"\\Registry\\Machine\\SOFTWARE\\CHIERU\\FileMon"


NTSTATUS QueryRegTest(PUNICODE_STRING pStrKeyName, PUNICODE_STRING pStrValul)//QueryRegTest(PUNICODE_STRING pStrKeyName[], PUNICODE_STRING pStrValul[], ULONG lenth)
{
	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;

	//初始化UNICODE_STRING字符串
	RtlInitUnicodeString(&RegUnicodeString, FILTER_KEY_NAME);


	OBJECT_ATTRIBUTES objectAttributes;
	//初始化objectAttributes
	InitializeObjectAttributes(&objectAttributes,
		&RegUnicodeString,
		OBJ_CASE_INSENSITIVE,//对大小写敏感
		NULL,
		NULL);
	//打开注册表
	NTSTATUS ntStatus = ZwOpenKey(&hRegister,
		KEY_ALL_ACCESS,
		&objectAttributes);

	if (NT_SUCCESS(ntStatus))
	{
		//KdPrint(("Open register successfully\n"));
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Open register successfully\n"));
	}
	UNICODE_STRING ValueName;
	ULONG ulSize;
	PKEY_VALUE_PARTIAL_INFORMATION pvpi = NULL;

	/*
	//初始化ValueName
	RtlInitUnicodeString(&ValueName, L"REG_DWORD test");

	//读取REG_DWORD子键
	ntStatus = ZwQueryValueKey(hRegister,
		&ValueName,
		KeyValuePartialInformation,
		NULL,
		0,
		&ulSize);

	if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0)
	{
		ZwClose(hRegister);
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("The item is not exist:%d\n", ulSize));
		return;
	}
	pvpi =
		(PKEY_VALUE_PARTIAL_INFORMATION)
		ExAllocatePool(PagedPool, ulSize);

	ntStatus = ZwQueryValueKey(hRegister,
		&ValueName,
		KeyValuePartialInformation,
		pvpi,
		ulSize,
		&ulSize);
	if (!NT_SUCCESS(ntStatus))
	{
		ZwClose(hRegister);
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Read regsiter error\n"));
		return;
	}
	//判断是否为REG_DWORD类型
	if (pvpi->Type == REG_DWORD && pvpi->DataLength == sizeof(ULONG))
	{
		PULONG pulValue = (PULONG)pvpi->Data;
		//KdPrint(("The value:%d\n", *pulValue));
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("The value:%d\n", *pulValue));
	}

	ExFreePool(pvpi);
	*/

	//初始化ValueName
	RtlInitUnicodeString(&ValueName, L"IgnoreProcess");
	//读取REG_SZ子键
	ntStatus = ZwQueryValueKey(hRegister,
		&ValueName,
		KeyValuePartialInformation,
		NULL,
		0,
		&ulSize);

	if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0)
	{
		ZwClose(hRegister);
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("The item is not exist:%d\n", ulSize));
		return -1;
	}
	pvpi = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(PagedPool, ulSize);

	ntStatus = ZwQueryValueKey(hRegister,
		&ValueName,
		KeyValuePartialInformation,
		pvpi,
		ulSize,
		&ulSize);
	if (!NT_SUCCESS(ntStatus))
	{
		ZwClose(hRegister);
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Read regsiter error\n"));
		return -1;
	}
	//判断是否为REG_SZ类型
	if (pvpi->Type == REG_SZ)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("The value : %S\n", pvpi->Data));

		STRING_HASH_MAP InitHashMap;
		StringHashInit(&InitHashMap, pvpi->Data, 0, 4);
	}

	ExFreePool(pvpi);
	ZwClose(hRegister);
}

NTSTATUS InitFilterRules()
{
	QueryRegTest(NULL,NULL);
	return 1;
}


//strlwr(); strupr
