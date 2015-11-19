#pragma once
//#ifndef __FLOGFUN_H__
//#define __FLOGFUN_H__

#include <fltKernel.h>
//#include <dontuse.h>
#include <suppress.h>



/*************************************************************************
Prototypes
*************************************************************************/
#pragma region 函数预定义
void EnumProcessInfo();
VOID CreateProcessNotifyRoutine(IN HANDLE  ParentId, IN HANDLE  ProcessId, IN BOOLEAN  Create);
NTSTATUS GetProcessImagePath(IN ULONG dwProcessId, OUT PUNICODE_STRING ProcessImagePath);
NTSTATUS GetUserName(IN ULONG dwProcessId, OUT PUNICODE_STRING ProcessUserName);
NTSTATUS GetProcessInfo(IN ULONG dwProcessId, OUT PUNICODE_STRING ProcessName, OUT PUNICODE_STRING ProcessUserName);


//#endif __FLOGFUN_H__