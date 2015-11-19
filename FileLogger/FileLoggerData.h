/*++

Copyright (c) 1989-2002  Microsoft Corporation

Module Name:

FSLogKern.h

Abstract:
Header file which contains the structures, type definitions,
constants, global variables and function prototypes that are
only visible within the kernel.

Environment:

Kernel mode

--*/
#pragma once
//#ifndef __FLOGDATA_H__
//#define __FLOGDATA_H__

#include <fltKernel.h>
//#include <dontuse.h>
#include <suppress.h>
#include "Container.h"
#include "FileLoggerFilter.h"

#define MAX_FILENAME	254
#define USE_ADVLOG_FORMAT	1


#define AllocatePoolTag	"FLOG"

//=========================================================================
#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
//ULONG gTraceFlags = PTDBG_TRACE_ROUTINES | PTDBG_TRACE_OPERATION_STATUS;
extern ULONG gTraceFlags;
#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))
//=========================================================================

//
//  The maximum size of a record that can be passed from the filter
//
#ifdef USE_ADVLOG_FORMAT 
#define RECORD_SIZE     2048
#else
#define RECORD_SIZE     512
#endif

#define DEFAULT_MAX_RECORDS_TO_ALLOCATE     8000//3000
#define FL_TAG 'FLTG'
#define FLOG_PORT_NAME			L"\\FLogPort"

typedef ULONG_PTR FILE_ID;


/*
typedef struct _SYSTEM_PROCESSES
{
ULONG NextEntryDelta; //构成结构序列的偏移量;
ULONG ThreadCount; //线程数目;
ULONG Reserved1[6];
LARGE_INTEGER CreateTime; //创建时间;
LARGE_INTEGER UserTime;//用户模式(Ring 3)的CPU时间;
LARGE_INTEGER KernelTime; //内核模式(Ring 0)的CPU时间;
UNICODE_STRING ProcessName; //进程名称;
KPRIORITY BasePriority;//进程优先权;
ULONG ProcessId; //进程标识符;
ULONG InheritedFromProcessId; //父进程的标识符;
ULONG HandleCount; //句柄数目;
ULONG Reserved2[2];
VM_COUNTERS  VmCounters; //虚拟存储器的结构，见下;
IO_COUNTERS IoCounters; //IO计数结构，见下;
SYSTEM_THREADS Threads[1]; //进程相关线程的结构数组
}SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;//*/
typedef struct _SYSTEM_PROCESSES
{
	ULONG NextEntryDelta;
	ULONG ThreadCount;
	ULONG Reserved[6];
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ProcessName;
	KPRIORITY BasePriority;
	ULONG ProcessId;
	ULONG InheritedFromProcessId;
	ULONG HandleCount;
	ULONG Reserved2[2];
	VM_COUNTERS VmCounters;
	IO_COUNTERS IoCounters;
} _SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;
typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation, // 0 Y N
	SystemProcessorInformation, // 1 Y N
	SystemPerformanceInformation, // 2 Y N
	SystemTimeOfDayInformation, // 3 Y N
	SystemNotImplemented1, // 4 Y N
	SystemProcessesAndThreadsInformation, // 5 Y N
	SystemCallCounts, // 6 Y N
	SystemConfigurationInformation, // 7 Y N
	SystemProcessorTimes, // 8 Y N
	SystemGlobalFlag, // 9 Y Y
	SystemNotImplemented2, // 10 Y N
	SystemModuleInformation, // 11 Y N
	SystemLockInformation, // 12 Y N
	SystemNotImplemented3, // 13 Y N
	SystemNotImplemented4, // 14 Y N
	SystemNotImplemented5, // 15 Y N
	SystemHandleInformation, // 16 Y N
	SystemObjectInformation, // 17 Y N
	SystemPagefileInformation, // 18 Y N
	SystemInstructionEmulationCounts, // 19 Y N
	SystemInvalidInfoClass1, // 20
	SystemCacheInformation, // 21 Y Y
	SystemPoolTagInformation, // 22 Y N
	SystemProcessorStatistics, // 23 Y N
	SystemDpcInformation, // 24 Y Y
	SystemNotImplemented6, // 25 Y N
	SystemLoadImage, // 26 N Y
	SystemUnloadImage, // 27 N Y
	SystemTimeAdjustment, // 28 Y Y
	SystemNotImplemented7, // 29 Y N
	SystemNotImplemented8, // 30 Y N
	SystemNotImplemented9, // 31 Y N
	SystemCrashDumpInformation, // 32 Y N
	SystemExceptionInformation, // 33 Y N
	SystemCrashDumpStateInformation, // 34 Y Y/N
	SystemKernelDebuggerInformation, // 35 Y N
	SystemContextSwitchInformation, // 36 Y N
	SystemRegistryQuotaInformation, // 37 Y Y
	SystemLoadAndCallImage, // 38 N Y
	SystemPrioritySeparation, // 39 N Y
	SystemNotImplemented10, // 40 Y N
	SystemNotImplemented11, // 41 Y N
	SystemInvalidInfoClass2, // 42
	SystemInvalidInfoClass3, // 43
	SystemTimeZoneInformation, // 44 Y N
	SystemLookasideInformation, // 45 Y N
	SystemSetTimeSlipEvent, // 46 N Y
	SystemCreateSession, // 47 N Y
	SystemDeleteSession, // 48 N Y
	SystemInvalidInfoClass4, // 49
	SystemRangeStartInformation, // 50 Y N
	SystemVerifierInformation, // 51 Y Y
	SystemAddVerifier, // 52 N Y
	SystemSessionProcessesInformation // 53 Y N
}SYSTEM_INFORMATION_CLASS;

NTSYSAPI NTSTATUS 
NTAPI ZwQuerySystemInformation(
						IN ULONG SystemInformationClass,
						IN OUT PVOID SystemInformation,
						IN ULONG SystemInformationLength,
						OUT PULONG ReturnLength);
NTSYSAPI NTSTATUS
NTAPI ZwQueryInformationProcess(
			IN HANDLE ProcessHandle, // 进程句柄
			IN PROCESSINFOCLASS InformationClass, // 信息类型
			OUT PVOID ProcessInformation, // 缓冲指针
			IN ULONG ProcessInformationLength, // 以字节为单位的缓冲大小
			OUT PULONG ReturnLength OPTIONAL // 写入缓冲的字节数
			);
typedef NTSTATUS(*QUERY_INFO_PROCESS) (
	__in HANDLE ProcessHandle,
	__in PROCESSINFOCLASS ProcessInformationClass,
	__out_bcount(ProcessInformationLength) PVOID ProcessInformation,
	__in ULONG ProcessInformationLength,
	__out_opt PULONG ReturnLength
	);

//
//  Defines the commands between the utility and the filter
//
typedef enum _FLOG_COMMAND {
	GetFileLog,
	GetFileLogVersion,
	SetFileLogFilePath,
	SetFileLogAttach,
} FLOG_COMMAND;

//
//  Defines the command structure between the utility and the filter.
//
typedef struct _COMMAND_MESSAGE {
	FLOG_COMMAND Command;
	ULONG Reserved;	//	Alignment on IA64
	UCHAR Data[1100];
} COMMAND_MESSAGE, *PCOMMAND_MESSAGE;

//
//  Defines the context structure
//

typedef struct _FLOG_TRANSACTION_CONTEXT {

	ULONG Count;

}FLOG_TRANSACTION_CONTEXT, *PFLOG_TRANSACTION_CONTEXT;

//
//  Version definition
//
#define FSLOG_MAJ_VERSION 0
#define FSLOG_MIN_VERSION 1
typedef struct _FLOGVER {
	USHORT Major;
	USHORT Minor;
} FLOGVER, *PFLOGVER;


//---------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------
// 
//

typedef struct _FLOG_DATA {

	//
	//  The object that identifies this driver.
	//

	PDRIVER_OBJECT DriverObject;

	//
	//  The filter that results from a call to
	//  FltRegisterFilter.
	//

	PFLT_FILTER Filter;

	//
	//  Server port: user mode connects to this port
	//

	PFLT_PORT ServerPort;

	//
	//  Client connection port: only one connection is allowed at a time.,
	//

	PFLT_PORT ClientPort;

	//
	//  List of buffers with data to send to user mode.
	//

	KSPIN_LOCK OutputBufferLock;
	LIST_ENTRY OutputBufferList;

	//
	//  Lookaside list used for allocating buffers.
	//

	NPAGED_LOOKASIDE_LIST FreeBufferList;

	//
	//  Variables used to throttle how many records buffer we can use
	//

	LONG MaxRecordsToAllocate;
	__volatile LONG RecordsAllocated;

	//
	//  static buffer used for sending an "out-of-memory" message
	//  to user mode.
	//

	__volatile ULONG StaticBufferInUse;

	//
	//  We need to make sure this buffer aligns on a PVOID boundary because
	//  FSLog casts this buffer to a RECORD_LIST structure.
	//  That can cause alignment faults unless the structure starts on the
	//  proper PVOID boundary
	//

	//PVOID OutOfMemoryBuffer[RECORD_SIZE / sizeof(PVOID)];//备用的一个内存，暂时不需要

	//
	//  Variable and lock for maintaining LogRecord sequence numbers.
	//

	__volatile ULONG LogSequenceNumber;

	//
	//  The name query method to use.  By default, it is set to
	//  FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, but it can be overridden
	//  by a setting in the registery.
	//

	ULONG NameQueryMethod;

	//
	//  Global debug flags
	//

	ULONG DebugFlags;

#if FLT_MGR_LONGHORN

	//
	//  Dynamically imported Filter Mgr APIs
	//

	NTSTATUS
		(*PFltSetTransactionContext)(
		__in PFLT_INSTANCE Instance,
		__in PKTRANSACTION Transaction,
		__in FLT_SET_CONTEXT_OPERATION Operation,
		__in PFLT_CONTEXT NewContext,
		__deref_opt_out PFLT_CONTEXT *OldContext
		);

	NTSTATUS
		(*PFltGetTransactionContext)(
		__in PFLT_INSTANCE Instance,
		__in PKTRANSACTION Transaction,
		__deref_out PFLT_CONTEXT *Context
		);

	NTSTATUS
		(*PFltEnlistInTransaction)(
		__in PFLT_INSTANCE Instance,
		__in PKTRANSACTION Transaction,
		__in PFLT_CONTEXT TransactionContext,
		__in NOTIFICATION_MASK NotificationMask
		);

#endif

} FLOG_DATA, *PFLOG_DATA;


