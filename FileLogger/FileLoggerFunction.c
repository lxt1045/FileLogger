#include "FileLoggerFunction.h"
#include "FileLoggerData.h"
//#include <suppress.h>


extern HASH_MAP g_hashMap;

void EnumProcessInfo()
{
	NTSTATUS status;
	ULONG size = 0x100000;
	PVOID Buffer = NULL;
	PSYSTEM_PROCESSES  SystemInformation = NULL;

	UNICODE_STRING strUser;
	WCHAR strBuffer1[260];

	strUser.Buffer = strBuffer1;
	strUser.Length = 0;
	strUser.MaximumLength = 260 * sizeof(WCHAR);

	Buffer = ExAllocatePoolWithTag(NonPagedPool, size, 'tag1');
	if (Buffer == 0)
	{
		DbgPrint("ExAllocatePool fail");
		return;
	}
	status = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, Buffer, size, NULL);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("ZwQuerySystemInformation fail");
		return;
	}

	SystemInformation = (PSYSTEM_PROCESSES)Buffer;
	while (1)
	{
		if (SystemInformation->ProcessId == 0)
		{
			KdPrint(("PID:%d system Idle Process\n", SystemInformation->ProcessId));
			//GetUserName(SystemInformation->ProcessId, NULL);//没有用户名
		}
		else
		{
			//KdPrint(("Process ID:%d,Process Name:%wZ\n", SystemInformation->ProcessId, &SystemInformation->ProcessName));
			GetUserName(SystemInformation->ProcessId, &strUser);
			//DbgPrint("ProcessUser:%wZ\n", &strUser);
			hashInsert(&g_hashMap, SystemInformation->ProcessId, &SystemInformation->ProcessName, &strUser);
		}

		if (SystemInformation->NextEntryDelta == 0)
		{
			break;
		}
		SystemInformation = (PSYSTEM_PROCESSES)(((PUCHAR)SystemInformation) + SystemInformation->NextEntryDelta);
	}

	ExFreePool(Buffer);
}

/*
typedef struct _EX_CALLBACK_ROUTINE_BLOCK
{
EX_RUNDOWN_REF RundownProtect;
PEX_CALLBACK_FUNCTION Function;
PVOID Context;
} EX_CALLBACK_ROUTINE_BLOCK, *PEX_CALLBACK_ROUTINE_BLOCK;//*/
VOID CreateProcessNotifyRoutine(IN HANDLE  ParentId, IN HANDLE  ProcessId, IN BOOLEAN  Create)
{
	NTSTATUS status;
	WCHAR strBuffer1[260];
	WCHAR strBuffer2[260];
	UNICODE_STRING strName, strUser;
	//str = (UNICODE_STRING*)&strBuffer;

	//initialize
	strName.Buffer = strBuffer1;
	strName.Length = 0;
	strName.MaximumLength = 260 * sizeof(WCHAR);

	strUser.Buffer = strBuffer2;
	strUser.Length = 0;
	strUser.MaximumLength = 260 * sizeof(WCHAR);

	PAGED_CODE();//此宏确保调用线程行在一个允许分页的足够低IRQL级别

	if (Create) {

		//DbgPrint("CreateProcessNotifyRoutine:Create\n");
		if (KeGetCurrentIrql() <= PASSIVE_LEVEL)
		{
			GetProcessInfo(ProcessId, &strName, &strUser);
			//DbgPrint("ProcessName:%wZ\n", &strName);
			//DbgPrint("ProcessUser:%wZ\n", &strUser);

			hashInsert(&g_hashMap, ProcessId, &strName, &strUser);
			//printfHashMap(&g_hashMap);
		}
		else
		{
			DbgPrint("CreateProcessNotifyRoutine: KeGetCurrentIrql() > PASSIVE_LEVEL\n");
		}
		return STATUS_PROCEDURE_NOT_FOUND;
	}
	else //如果要注册
	{
		//DbgPrint("CreateProcessNotifyRoutine:!Create\n");
		hashRemove(&g_hashMap, ProcessId);
		return STATUS_INVALID_PARAMETER;
	}

}

NTSTATUS GetProcessImagePath(IN ULONG dwProcessId, OUT PUNICODE_STRING ProcessImagePath)
{
	NTSTATUS Status;
	HANDLE  hProcess;
	PEPROCESS pEprocess;
	ULONG  returnedLength;
	ULONG  bufferLength;
	PVOID  buffer;
	PUNICODE_STRING imageName;

	PAGED_CODE();  // this eliminates the possibility of the IDLE Thread/Process   

	/*
	QUERY_INFO_PROCESS ZwQueryInformationProcess2;
	if (NULL == ZwQueryInformationProcess2) {

	ZwQueryInformationProcess2 = ZwQueryInformationProcess;
	/*UNICODE_STRING routineName;
	RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");
	DbgPrint("GetProcessImagePath:%wZ\n", routineName);

	ZwQueryInformationProcess2 =
	(QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);

	if (NULL == ZwQueryInformationProcess2) {
	DbgPrint("Cannot resolve ZwQueryInformationProcess/n");
	}*//*
	}//*/
	Status = PsLookupProcessByProcessId((HANDLE)dwProcessId, &pEprocess);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = ObOpenObjectByPointer(pEprocess,           // Object   
		OBJ_KERNEL_HANDLE,   // HandleAttributes   
		NULL,                // PassedAccessState OPTIONAL   
		GENERIC_READ,        // DesiredAccess   
		*PsProcessType,      // ObjectType   
		KernelMode,          // AccessMode   
		&hProcess);
	if (!NT_SUCCESS(Status))
		return Status;//

	//   
	// Step one - get the size we need   
	//   
	//hProcess = PsGetCurrentProcess(); //ZwQueryInformationProcess不能正常获取
	//hProcess = NtCurrentProcess(); //获取时有时出错，可能是ZwQueryInformationProcess调用时，上下文已经不是调用的那个process了！
	Status = ZwQueryInformationProcess(hProcess,
		ProcessImageFileName,
		NULL,  // buffer   
		0,  // buffer size   
		&returnedLength);


	if (STATUS_INFO_LENGTH_MISMATCH != Status) {
		return  Status;
	}

	//   
	// Is the passed-in buffer going to be big enough for us?    
	// This function returns a single contguous buffer model...   
	//   
	bufferLength = returnedLength - sizeof(UNICODE_STRING);
	if (ProcessImagePath->MaximumLength < bufferLength) {
		ProcessImagePath->Length = (USHORT)bufferLength;
		return  STATUS_BUFFER_OVERFLOW;
	}

	//   
	// If we get here, the buffer IS going to be big enough for us, so   
	// let's allocate some storage.   
	//   
	buffer = ExAllocatePoolWithTag(PagedPool, returnedLength, 'ipgD');
	if (NULL == buffer) {
		return  STATUS_INSUFFICIENT_RESOURCES;
	}

	//   
	// Now lets go get the data   
	//   
	Status = ZwQueryInformationProcess(hProcess,
		ProcessImageFileName,
		buffer,
		returnedLength,
		&returnedLength);

	if (NT_SUCCESS(Status)) {
		//   
		// Ah, we got what we needed   
		//   
		imageName = (PUNICODE_STRING)buffer;
		if (ProcessImagePath != NULL)
			RtlCopyUnicodeString(ProcessImagePath, imageName);
		else
			//DbgPrint("GetProcessImagePath,imageName:%wZ\n", imageName);
			DbgPrint("GetProcessImagePath,ProcessImagePath:%wZ\n", ProcessImagePath);
	}

	ZwClose(hProcess);

	//   
	// free our buffer   
	//   
	ExFreePool(buffer);


	//   
	// And tell the caller what happened.   
	//      
	return  Status;

}

NTSTATUS GetUserName(IN ULONG dwProcessId, OUT PUNICODE_STRING ProcessUserName)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE         hProcess;
	PEPROCESS pEprocess;
	HANDLE         TokenHandle;
	ULONG         ReturnLength;
	ULONG       size;
	PTOKEN_USER TokenInformation;
	//WCHAR SidStringBuffer[260];
	//WCHAR SidStringBuffer2[260];

	PAGED_CODE();  // this eliminates the possibility of the IDLE Thread/Process   

	status = PsLookupProcessByProcessId((HANDLE)dwProcessId, &pEprocess);
	if (!NT_SUCCESS(status))
		return status;

	status = ObOpenObjectByPointer(pEprocess,           // Object   
		OBJ_KERNEL_HANDLE,   // HandleAttributes   
		NULL,                // PassedAccessState OPTIONAL   
		GENERIC_READ,        // DesiredAccess   
		*PsProcessType,      // ObjectType   
		KernelMode,          // AccessMode   
		&hProcess);
	if (!NT_SUCCESS(status))
		return status;//

	status = ZwOpenProcessTokenEx(hProcess, TOKEN_READ, OBJ_KERNEL_HANDLE, &TokenHandle);//NtCurrentProcess(),
	if (!NT_SUCCESS(status)) {
		return status;
	}

	// 获取Sid 
	{
		status = ZwQueryInformationToken(TokenHandle, TokenUser, NULL, 0, &ReturnLength);
		if (STATUS_BUFFER_TOO_SMALL != status)
		{
			KdPrint(("QueryLogonSID::ZwQueryInformationToken #1 failed: %08X\n", status));
			return status;
		}

		TokenInformation = (TOKEN_GROUPS *)ExAllocatePool(NonPagedPool, ReturnLength);
		if (NULL == TokenInformation)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			KdPrint(("QueryLogonSID::ExAllocatePool failed: %08X\n", status));
			//ExFreePool(tokenGroups);
			return status;
		}

		status = ZwQueryInformationToken(TokenHandle, TokenUser, TokenInformation, ReturnLength, &ReturnLength);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("QueryLogonSID::ZwQueryInformationToken #2 failed: %08X\n", status));
			return status;
		}
	}
	//由Sid得到用户名
	{
		UNICODE_STRING  UstrName;
		UNICODE_STRING UstrDomain;
		ULONG dwAcctName = 1, dwDomainName = 1;
		SID_NAME_USE eUse = SidTypeUnknown;
		PSID Sid = ((PTOKEN_USER)TokenInformation)->User.Sid;

		//RtlZeroMemory(&UstrName, sizeof(UNICODE_STRING));
		//RtlZeroMemory(&UstrDomain, sizeof(UNICODE_STRING));//*/
		status = SecLookupAccountSid(Sid, &dwAcctName, NULL, &dwDomainName, NULL, &eUse);
		if (status == STATUS_BUFFER_TOO_SMALL)
		{
			//*
			UstrName.MaximumLength = dwAcctName + 2; /// for the L'\0'
			UstrName.Length = 0;
			UstrName.Buffer = ExAllocatePoolWithTag(PagedPool, UstrName.MaximumLength, 'tap1');
			//RtlZeroMemory(UstrName.Buffer, UstrName.MaximumLength*sizeof(WCHAR));

			UstrDomain.MaximumLength = dwDomainName + 2; /// for the '\0'
			UstrDomain.Length = 0;
			UstrDomain.Buffer = ExAllocatePoolWithTag(PagedPool, UstrDomain.MaximumLength, 'tap1');//*/

			//RtlZeroMemory(SidStringBuffer, sizeof(SidStringBuffer));
			//UstrName.Buffer = (PWCHAR)SidStringBuffer;
			//UstrName.MaximumLength = 259;// sizeof(SidStringBuffer);
			//UstrDomain.Length = 0;
			//
			//RtlZeroMemory(SidStringBuffer2, sizeof(SidStringBuffer2));
			//UstrDomain.Buffer = (PWCHAR)SidStringBuffer2;
			//UstrDomain.MaximumLength = 259;//sizeof(SidStringBuffer2);
			//UstrDomain.Length = 0;

			//if (dwAccName>0 && dwAccName < MAX_PATH && dwDomainName>0 && dwDomainName <= MAX_PATH)
			if (UstrName.Buffer != NULL && UstrDomain.Buffer != NULL)
			{
				status = SecLookupAccountSid(Sid, &dwAcctName, &UstrName, &dwDomainName, &UstrDomain, &eUse);

				if (status == STATUS_BUFFER_TOO_SMALL)
				{
					DbgPrint("SecLookupAccountSid:Memery Too Small!\n");
				}
				else if (NT_SUCCESS(status))
				{
					/*DbgPrint("SecLookupAccountSid: %wZ\n", &UstrName);
					DbgPrint("SecLookupAccountSid: %wZ\n", &UstrDomain);*/
					if (ProcessUserName != NULL)
						RtlCopyUnicodeString(ProcessUserName, &UstrName);
					else
						DbgPrint("SecLookupAccountSid: %wZ\n", &UstrName);
				}
				else
				{
					DbgPrint("SecLookupAccountSid,errorCode: %ud\n", status);
				}
				ExFreePool(UstrName.Buffer);
				ExFreePool(UstrDomain.Buffer);
			}
			else
			{
				ExFreePool(TokenInformation);
				return status;
			}
		}
		else
		{
			ExFreePool(TokenInformation);
			return status;
		}
	}

	ZwClose(TokenHandle);

	/*
	{
	UNICODE_STRING SidString;
	WCHAR SidStringBuffer[260];
	RtlZeroMemory(SidStringBuffer, sizeof(SidStringBuffer));
	SidString.Buffer = (PWCHAR)SidStringBuffer;
	SidString.MaximumLength = sizeof(SidStringBuffer);

	status = RtlConvertSidToUnicodeString(&SidString, ((PTOKEN_USER)TokenInformation)->User.Sid, FALSE);
	DbgPrint("sudamis PC Name: %wZ\n", &SidString);
	}//*/

	ExFreePool(TokenInformation);

	return STATUS_SUCCESS;

	//ERROR_CLEANUP:
	//	if (TokenInformation != NULL)
	//		ExFreePool(TokenInformation);
	//	return status;
}

NTSTATUS GetProcessInfo(IN ULONG dwProcessId, OUT PUNICODE_STRING ProcessName, OUT PUNICODE_STRING ProcessUserName)
{
	NTSTATUS status;
	HANDLE  hProcess;
	PEPROCESS pEprocess;
	ULONG  returnedLength;
	ULONG  bufferLength;
	PVOID  buffer;
	PUNICODE_STRING imageName;

	PAGED_CODE();  // this eliminates the possibility of the IDLE Thread/Process   

	status = PsLookupProcessByProcessId((HANDLE)dwProcessId, &pEprocess);
	if (!NT_SUCCESS(status))
		return status;

	status = ObOpenObjectByPointer(pEprocess,           // Object   
		OBJ_KERNEL_HANDLE,   // HandleAttributes   
		NULL,                // PassedAccessState OPTIONAL   
		GENERIC_READ,        // DesiredAccess   
		*PsProcessType,      // ObjectType   
		KernelMode,          // AccessMode   
		&hProcess);
	if (!NT_SUCCESS(status))
		return status;//

	{
		status = ZwQueryInformationProcess(hProcess,
			ProcessImageFileName,
			NULL,  // buffer   
			0,  // buffer size   
			&returnedLength);

		if (STATUS_INFO_LENGTH_MISMATCH != status) {
			return  status;
		}

		//   
		// If we get here, the buffer IS going to be big enough for us, so   
		// let's allocate some storage.   
		//   
		buffer = ExAllocatePoolWithTag(PagedPool, returnedLength, 'ipgD');
		if (NULL == buffer) {
			return  STATUS_INSUFFICIENT_RESOURCES;
		}

		//   
		// Now lets go get the data   
		//   
		status = ZwQueryInformationProcess(hProcess,
			ProcessImageFileName,
			buffer,
			returnedLength,
			&returnedLength);

		if (NT_SUCCESS(status)) {
			imageName = (PUNICODE_STRING)buffer;

			WCHAR *lpTemp = wcsrchr(imageName->Buffer, L'\\');
			if (lpTemp)
			{
				imageName->Length -= (imageName->Buffer - lpTemp - 1)*sizeof(WCHAR);//byte
				imageName->Buffer = lpTemp + 1;
			}
			if (ProcessName != NULL)
			{
				RtlCopyUnicodeString(ProcessName, imageName);
				//DbgPrint("GetProcessImagePath,1:%wZ\n", imageName);
			}
			else
				DbgPrint("GetProcessImagePath,2:%wZ\n", ProcessName);
		}
		ExFreePool(buffer);
	}
	{
		PTOKEN_USER TokenInformation;
		HANDLE         TokenHandle;
		ULONG         ReturnLength;

		status = ZwOpenProcessTokenEx(hProcess, TOKEN_READ, OBJ_KERNEL_HANDLE, &TokenHandle);//NtCurrentProcess(),
		if (!NT_SUCCESS(status)) {
			return status;
		}

		// 获取Sid 
		{
			status = ZwQueryInformationToken(TokenHandle, TokenUser, NULL, 0, &ReturnLength);
			if (STATUS_BUFFER_TOO_SMALL != status)
			{
				KdPrint(("QueryLogonSID::ZwQueryInformationToken #1 failed: %08X\n", status));
				return status;
			}

			TokenInformation = (TOKEN_GROUPS *)ExAllocatePool(NonPagedPool, ReturnLength);
			if (NULL == TokenInformation)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				KdPrint(("QueryLogonSID::ExAllocatePool failed: %08X\n", status));
				//ExFreePool(tokenGroups);
				return status;
			}

			status = ZwQueryInformationToken(TokenHandle, TokenUser, TokenInformation, ReturnLength, &ReturnLength);
			if (!NT_SUCCESS(status))
			{
				KdPrint(("QueryLogonSID::ZwQueryInformationToken #2 failed: %08X\n", status));
				return status;
			}
		}
		//由Sid得到用户名
		{
			UNICODE_STRING  UstrName;
			UNICODE_STRING UstrDomain;
			ULONG dwAcctName = 1, dwDomainName = 1;
			SID_NAME_USE eUse = SidTypeUnknown;
			PSID Sid = ((PTOKEN_USER)TokenInformation)->User.Sid;

			//RtlZeroMemory(&UstrName, sizeof(UNICODE_STRING));
			//RtlZeroMemory(&UstrDomain, sizeof(UNICODE_STRING));//*/
			status = SecLookupAccountSid(Sid, &dwAcctName, NULL, &dwDomainName, NULL, &eUse);
			if (status == STATUS_BUFFER_TOO_SMALL)
			{
				//*
				UstrName.MaximumLength = dwAcctName + 2; /// for the L'\0'
				UstrName.Length = 0;
				UstrName.Buffer = ExAllocatePoolWithTag(PagedPool, UstrName.MaximumLength, 'tap1');
				//RtlZeroMemory(UstrName.Buffer, UstrName.MaximumLength*sizeof(WCHAR));

				UstrDomain.MaximumLength = dwDomainName + 2; /// for the '\0'
				UstrDomain.Length = 0;
				UstrDomain.Buffer = ExAllocatePoolWithTag(PagedPool, UstrDomain.MaximumLength, 'tap1');//*/

				//if (dwAccName>0 && dwAccName < MAX_PATH && dwDomainName>0 && dwDomainName <= MAX_PATH)
				if (UstrName.Buffer != NULL && UstrDomain.Buffer != NULL)
				{
					status = SecLookupAccountSid(Sid, &dwAcctName, &UstrName, &dwDomainName, &UstrDomain, &eUse);

					if (status == STATUS_BUFFER_TOO_SMALL)
					{
						DbgPrint("SecLookupAccountSid:Memery Too Small!\n");
					}
					else if (NT_SUCCESS(status))
					{
						/*DbgPrint("SecLookupAccountSid: %wZ\n", &UstrName);
						DbgPrint("SecLookupAccountSid: %wZ\n", &UstrDomain);*/
						if (ProcessUserName != NULL)
							RtlCopyUnicodeString(ProcessUserName, &UstrName);
						else
							DbgPrint("SecLookupAccountSid: %wZ\n", &UstrName);
					}
					else
					{
						DbgPrint("SecLookupAccountSid,errorCode: %ud\n", status);
					}
					ExFreePool(UstrName.Buffer);
					ExFreePool(UstrDomain.Buffer);
				}
				else
				{
					ExFreePool(TokenInformation);
					return status;
				}
			}
			else
			{
				ExFreePool(TokenInformation);
				return status;
			}
		}

		ZwClose(TokenHandle);

		/*
		{
		UNICODE_STRING SidString;
		WCHAR SidStringBuffer[260];
		RtlZeroMemory(SidStringBuffer, sizeof(SidStringBuffer));
		SidString.Buffer = (PWCHAR)SidStringBuffer;
		SidString.MaximumLength = sizeof(SidStringBuffer);

		status = RtlConvertSidToUnicodeString(&SidString, ((PTOKEN_USER)TokenInformation)->User.Sid, FALSE);
		DbgPrint("sudamis PC Name: %wZ\n", &SidString);
		}//*/

		ExFreePool(TokenInformation);
	}
	ZwClose(hProcess);

	return  status;
}