/*++

Module Name:

    FileLogger.c

Abstract:

    This is the main module of the FileLogger miniFilter driver.

Environment:

    Kernel mode

--*/

//#include "FileLoggerFunction.h"
//#include <fltKernel.h>
//#include <dontuse.h>
//#include <suppress.h>

//#include "c1.h" 
#include "FileLoggerData.h"
#include "FileLoggerFunction.h"
#include "ntifs.h" 
//#pragma comment(lib,"Ksecdd.lib") //加入链接库

//PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

ULONG gTraceFlags = PTDBG_TRACE_ROUTINES | PTDBG_TRACE_OPERATION_STATUS;

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

/*************************************************************************
    Prototypes
*************************************************************************/
#pragma region 函数预定义

NTSTATUS
FileLoggerInstanceSetup(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
_In_ DEVICE_TYPE VolumeDeviceType,
_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
FileLoggerInstanceTeardownStart(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
FileLoggerInstanceTeardownComplete(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
FileLoggerUnload(
_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
FileLoggerInstanceQueryTeardown(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);


FLT_PREOP_CALLBACK_STATUS
FileLoggerPreOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

VOID
FileLoggerOperationStatusCallback(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
_In_ NTSTATUS OperationStatus,
_In_ PVOID RequesterContext
);

FLT_POSTOP_CALLBACK_STATUS
FileLoggerPostOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID CompletionContext,
_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FileLoggerPreOperationNoPostOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

BOOLEAN
FileLoggerDoRequestOperationStatus(
_In_ PFLT_CALLBACK_DATA Data
);

#if FLT_MGR_LONGHORN
//  清理上下文
VOID
FileLoggerDeleteContext(
__inout PFLT_CONTEXT  Context,
__in FLT_CONTEXT_TYPE  ContextType
);
#endif
//
//  Assign text sections for each routine.
//
#if FLT_MGR_LONGHORN
NTSTATUS
FileLoggerKtmNotificationCallback(
__in PCFLT_RELATED_OBJECTS FltObjects,
__in PFLT_CONTEXT TransactionContext,
__in ULONG TransactionNotification
);
#endif


NTSTATUS
FileLoggerMessage(
__in PVOID ConnectionCookie,
__in_bcount_opt(InputBufferSize) PVOID InputBuffer,
__in ULONG InputBufferSize,
__out_bcount_part_opt(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
__in ULONG OutputBufferSize,
__out PULONG ReturnOutputBufferLength
);

NTSTATUS
FileLoggerConnect(
__in PFLT_PORT ClientPort,
__in PVOID ServerPortCookie,
__in_bcount(SizeOfContext) PVOID ConnectionContext,
__in ULONG SizeOfContext,
__deref_out_opt PVOID *ConnectionCookie
);

VOID
FileLoggerDisconnect(
__in_opt PVOID ConnectionCookie
);


DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );
#pragma endregion

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FileLoggerUnload)
#pragma alloc_text(PAGE, FileLoggerInstanceQueryTeardown)
#pragma alloc_text(PAGE, FileLoggerInstanceSetup)
#pragma alloc_text(PAGE, FileLoggerInstanceTeardownStart)
#pragma alloc_text(PAGE, FileLoggerInstanceTeardownComplete)
#endif

#pragma region 
#pragma endregion
#pragma region 一些上下文需求
//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

#if 1 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_CLOSE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_READ,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_WRITE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_SET_EA,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      FileLoggerPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_PNP,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

/*    { IRP_MJ_NOTIFY_STREAM_FILE_OBJECT,
	  0,
	  FileLoggerPreOperation,
	  FslPostOperationCallback },//*/

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      FileLoggerPreOperation,
	  FileLoggerPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      FileLoggerPreOperation,
      FileLoggerPostOperation },

#endif // TODO

    { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION Contexts[] = {

#if FLT_MGR_LONGHORN

	{ FLT_TRANSACTION_CONTEXT,
	0,
	FileLoggerDeleteContext,
	sizeof(FLOG_TRANSACTION_CONTEXT),
	'Lixt' },

#endif

	{ FLT_CONTEXT_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

	Contexts,                               //  Context
    Callbacks,                          //  Operation callbacks

    FileLoggerUnload,                           //  MiniFilterUnload

    FileLoggerInstanceSetup,                    //  InstanceSetup
    FileLoggerInstanceQueryTeardown,            //  InstanceQueryTeardown
    FileLoggerInstanceTeardownStart,            //  InstanceTeardownStart
    FileLoggerInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

#if FLT_MGR_LONGHORN
	,
	FileLoggerKtmNotificationCallback              //  KTM notification callback

#endif // FSLOG_LONGHORN
#if FLT_MGR_WIN8
	,
	NULL              //  KTM notification callback

#endif // FSLOG_LONGHORN
};

#pragma endregion

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/
//
//  Global variables
//

FLOG_DATA				FLogData;

NTSTATUS StatusToBreakOn = 0;

WCHAR logfilePath[MAX_FILENAME] = L"";
HASH_MAP g_hashMap;

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;

	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;
	int i;
    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,  ("FileLogger!DriverEntry: Entered\n") );

	try {

		//
		// Initialize global data structures.
		//

		FLogData.LogSequenceNumber = 0;
		FLogData.MaxRecordsToAllocate = DEFAULT_MAX_RECORDS_TO_ALLOCATE;
		FLogData.RecordsAllocated = 0;
		FLogData.NameQueryMethod = FLT_FILE_NAME_QUERY_DEFAULT;

		FLogData.DriverObject = DriverObject;

		InitializeListHead(&FLogData.OutputBufferList);
		KeInitializeSpinLock(&FLogData.OutputBufferLock);

		ExInitializeNPagedLookasideList(&FLogData.FreeBufferList,
			NULL,
			NULL,
			0,
			RECORD_SIZE,
			FL_TAG,
			0);

#if FLT_MGR_LONGHORN

		//
		//  Dynamically import FilterMgr APIs for transaction support
		//

		FLogData.PFltSetTransactionContext = FltGetRoutineAddress("FltSetTransactionContext");
		FLogData.PFltGetTransactionContext = FltGetRoutineAddress("FltGetTransactionContext");
		FLogData.PFltEnlistInTransaction = FltGetRoutineAddress("FltEnlistInTransaction");

#endif

		///////////////////////////////////////////////

    //
    //  Register with FltMgr to tell it our callback routines
    //
    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
								&FLogData.Filter);

	if (!NT_SUCCESS(status)) {
		leave;//out from to finally
	}

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (!NT_SUCCESS(status)) {
		leave;
	}
	
	RtlInitUnicodeString(&uniString, FLOG_PORT_NAME);

	InitializeObjectAttributes(&oa,
		&uniString,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		sd);

	//in FileLoggerUnload : must call FltCloseCommunicationPort befor FltUnregisterFilter
	status = FltCreateCommunicationPort(FLogData.Filter,
		&FLogData.ServerPort,
		&oa,
		NULL,
		FileLoggerConnect,
		FileLoggerDisconnect,
		FileLoggerMessage,
		1);

	FltFreeSecurityDescriptor(sd);

	if (!NT_SUCCESS(status)) {
		leave;
	}

	//
	//  Start filtering i/o
	//

	//status = FltStartFiltering(FLogData.Filter);
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileLogger!DriverEntry: FltStartFiltering...\n"));
	///////////////////////////////////////////////
	}
	finally {

		if (!NT_SUCCESS(status)) {

			if (NULL != FLogData.ServerPort) {
				FltCloseCommunicationPort(FLogData.ServerPort);
			}

			if (NULL != FLogData.Filter) {
				FltUnregisterFilter(FLogData.Filter);
			}

			ExDeleteNPagedLookasideList(&FLogData.FreeBufferList);
		}
	}



	if (hashInit(&g_hashMap, 64) >= 0)//256
	{
		EnumProcessInfo();
		PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, FALSE);

		//printfHashMap(&g_hashMap);
		//hashDestroy(&g_hashMap);
	}

    return status;
}

/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
FileLoggerPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;

	FILE_ID ProcessId;
	FILE_ID ThreadId; 

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("-------------------FileLogger!FileLoggerPreOperation: Entered-------------------\n") );


	ProcessId = (FILE_ID)PsGetCurrentProcessId();
	ThreadId = (FILE_ID)PsGetCurrentThreadId();


	//PEPROCESS pEprocess;//NtosKrnl.lib
	//pEprocess = PsGetCurrentProcess();//PEPROCESS  IoGetCurrentProcess(void);
	//DbgPrint("=======================ImageFileName: %.16s\n", PsGetProcessImageFileName(pProcess));

	



	
    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    if (FileLoggerDoRequestOperationStatus( Data )) {

        status = FltRequestOperationStatusCallback( Data,
                                                    FileLoggerOperationStatusCallback,
                                                    (PVOID)(++OperationStatusCtx) );
        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("FileLogger!FileLoggerPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
                           status) );
        }
    }

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
FileLoggerPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileLogger!FileLoggerPostOperation: Entered\n") );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FileLoggerPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileLogger!FileLoggerPreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
FileLoggerDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //


    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}


NTSTATUS
FileLoggerInstanceSetup(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
_In_ DEVICE_TYPE VolumeDeviceType,
_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/*++

Routine Description:

This routine is called whenever a new instance is created on a volume. This
gives us a chance to decide if we need to attach to this volume or not.

If this routine is not defined in the registration structure, automatic
instances are always created.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Flags describing the reason for this attach request.

Return Value:

STATUS_SUCCESS - attach
STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileLogger!FileLoggerInstanceSetup: Entered\n"));
	//DbgPrint("-----------FslInstanceSetup runing=====\n");

	return STATUS_SUCCESS;
}


NTSTATUS
FileLoggerInstanceQueryTeardown(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This is called when an instance is being manually deleted by a
call to FltDetachVolume or FilterDetach thereby giving us a
chance to fail that detach request.

If this routine is not defined in the registration structure, explicit
detach requests via FltDetachVolume or FilterDetach will always be
failed.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Indicating where this detach request came from.

Return Value:

Returns the status of this operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileLogger!FileLoggerInstanceQueryTeardown: Entered\n"));

	return STATUS_SUCCESS;
}


VOID
FileLoggerInstanceTeardownStart(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This routine is called at the start of instance teardown.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Reason why this instance is being deleted.

Return Value:

None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileLogger!FileLoggerInstanceTeardownStart: Entered\n"));
}


VOID
FileLoggerInstanceTeardownComplete(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This routine is called at the end of instance teardown.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Reason why this instance is being deleted.

Return Value:

None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileLogger!FileLoggerInstanceTeardownComplete: Entered\n"));
}


NTSTATUS
FileLoggerUnload(
_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
/*++

Routine Description:

This is the unload routine for this miniFilter driver. This is called
when the minifilter is about to be unloaded. We can fail this unload
request if this is not a mandatory unload indicated by the Flags
parameter.

Arguments:

Flags - Indicating if this is a mandatory unload.

Return Value:

Returns STATUS_SUCCESS.

--*/
{
	UNREFERENCED_PARAMETER(Flags);
	PAGED_CODE();
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileLogger!FileLoggerUnload: Entered\n"));

	FltCloseCommunicationPort(FLogData.ServerPort);
	FltUnregisterFilter(FLogData.Filter);
	//FslEmptyOutputBufferList();
	ExDeleteNPagedLookasideList(&FLogData.FreeBufferList);

	PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, TRUE);//delete

	printfHashMap(&g_hashMap);
	hashDestroy(&g_hashMap);
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileLogger!FileLoggerUnload: hashDestroy(&g_hashMap) Succes!\n"));

	return STATUS_SUCCESS;
}


VOID
FileLoggerOperationStatusCallback(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
_In_ NTSTATUS OperationStatus,
_In_ PVOID RequesterContext
)
/*++

Routine Description:

This routine is called when the given operation returns from the call
to IoCallDriver.  This is useful for operations where STATUS_PENDING
means the operation was successfully queued.  This is useful for OpLocks
and directory change notification operations.

This callback is called in the context of the originating thread and will
never be called at DPC level.  The file object has been correctly
referenced so that you can access it.  It will be automatically
dereferenced upon return.

This is non-pageable because it could be called on the paging path

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

RequesterContext - The context for the completion routine for this
operation.

OperationStatus -

Return Value:

The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileLogger!FileLoggerOperationStatusCallback: Entered\n"));

	PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
		("FileLogger!FileLoggerOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
		OperationStatus,
		RequesterContext,
		ParameterSnapshot->MajorFunction,
		ParameterSnapshot->MinorFunction,
		FltGetIrpName(ParameterSnapshot->MajorFunction)));
}


#if FLT_MGR_LONGHORN
VOID
FileLoggerDeleteContext(
__inout PFLOG_TRANSACTION_CONTEXT Context,
__in FLT_CONTEXT_TYPE ContextType
)
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(ContextType);

	ASSERT(FLT_TRANSACTION_CONTEXT == ContextType);
	ASSERT(Context->Count != 0);
}

#endif
#if FLT_MGR_LONGHORN

NTSTATUS
FileLoggerKtmNotificationCallback(
__in PCFLT_RELATED_OBJECTS FltObjects,
__in PFLT_CONTEXT TransactionContext,
__in ULONG TransactionNotification
)
{
	/*
	PRECORD_LIST recordList;

	//
	//  Try and get a log record
	//

	recordList = FslNewRecord();

	if (recordList) {

	FslLogTransactionNotify(FltObjects, recordList, TransactionNotification);

	//
	//  Send the logged information to the user service.
	//

	FslLog(recordList);
	}
	//*/

	return STATUS_SUCCESS;
}

#endif

NTSTATUS
FileLoggerConnect(
__in PFLT_PORT ClientPort,
__in PVOID ServerPortCookie,
__in_bcount(SizeOfContext) PVOID ConnectionContext,
__in ULONG SizeOfContext,
__deref_out_opt PVOID *ConnectionCookie
)
/*++

Routine Description

This is called when user-mode connects to the server
port - to establish a connection

Arguments

ClientPort - This is the pointer to the client port that
will be used to send messages from the filter.
ServerPortCookie - unused
ConnectionContext - unused
SizeofContext   - unused
ConnectionCookie - unused

Return Value

STATUS_SUCCESS - to accept the connection
--*/
{

	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ASSERT(FLogData.ClientPort == NULL);
	FLogData.ClientPort = ClientPort;

	//	FslHookAllDrives (TRUE);

	return STATUS_SUCCESS;
}


VOID
FileLoggerDisconnect(
__in_opt PVOID ConnectionCookie
)
/*++

Routine Description

This is called when the connection is torn-down. We use it to close our handle to the connection

Arguments

ConnectionCookie - unused

Return value

None
--*/
{

	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	//	FslHookAllDrives (FALSE);

	//
	//  Close our handle
	//
	if(FLogData.ClientPort != NULL)
		FltCloseClientPort(FLogData.Filter, &FLogData.ClientPort);
}


NTSTATUS
FileLoggerMessage(
__in PVOID ConnectionCookie,
__in_bcount_opt(InputBufferSize) PVOID InputBuffer,
__in ULONG InputBufferSize,
__out_bcount_part_opt(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
__in ULONG OutputBufferSize,
__out PULONG ReturnOutputBufferLength
)
/*++

Routine Description:

This is called whenever a user mode application wishes to communicate
with this minifilter.

Arguments:

ConnectionCookie - unused

OperationCode - An identifier describing what type of message this
is.  These codes are defined by the MiniFilter.
InputBuffer - A buffer containing input data, can be NULL if there
is no input data.
InputBufferSize - The size in bytes of the InputBuffer.
OutputBuffer - A buffer provided by the application that originated
the communication in which to store data to be returned to this
application.
OutputBufferSize - The size in bytes of the OutputBuffer.
ReturnOutputBufferSize - The size in bytes of meaningful data
returned in the OutputBuffer.

Return Value:

Returns the status of processing the message.

--*/
{

	FLOG_COMMAND command;
	NTSTATUS status;
	BOOLEAN bAttach;

	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	//
	//                      **** PLEASE READ ****
	//
	//  The INPUT and OUTPUT buffers are raw user mode addresses.  The filter
	//  manager has already done a ProbedForRead (on InputBuffer) and
	//  ProbedForWrite (on OutputBuffer) which guarentees they are valid
	//  addresses based on the access (user mode vs. kernel mode).  The
	//  minifilter does not need to do their own probe.
	//
	//  The filter manager is NOT doing any alignment checking on the pointers.
	//  The minifilter must do this themselves if they care (see below).
	//
	//  The minifilter MUST continue to use a try/except around any access to
	//  these buffers.
	//


	//DbgPrint("[LogVFileMonDrv.sys]: FslMessage ... ...\n");

	if ((InputBuffer != NULL) &&
		(InputBufferSize >= (FIELD_OFFSET(COMMAND_MESSAGE, Command) + sizeof(FLOG_COMMAND)))) {
		try  {
			//
			//  Probe and capture input message: the message is raw user mode
			//  buffer, so need to protect with exception handler
			//
			command = ((PCOMMAND_MESSAGE)InputBuffer)->Command;
		} except(EXCEPTION_EXECUTE_HANDLER) {
			return GetExceptionCode();		}

		switch (command) {
		case GetFileLog:
			//
			//  Return as many log records as can fit into the OutputBuffer
			//
			if ((OutputBuffer == NULL) || (OutputBufferSize == 0)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			//
			//  We want to validate that the given buffer is POINTER
			//  aligned.  But if this is a 64bit system and we want to
			//  support 32bit applications we need to be careful with how
			//  we do the check.  Note that the way FslGetLog is written
			//  it actually does not care about alignment but we are
			//  demonstrating how to do this type of check.
			//
#if defined(_WIN64)
			if (IoIs32bitProcess(NULL)) {
				//
				//  Validate alignment for the 32bit process on a 64bit
				//  system
				//
				if (!IS_ALIGNED(OutputBuffer, sizeof(ULONG))) {
					status = STATUS_DATATYPE_MISALIGNMENT;
					break;
				}
			}
			else {
#endif
				if (!IS_ALIGNED(OutputBuffer, sizeof(PVOID))) {
					status = STATUS_DATATYPE_MISALIGNMENT;
					break;
				}
#if defined(_WIN64)
			}
#endif
			//status = GetFileLog(OutputBuffer, OutputBufferSize, ReturnOutputBufferLength);
			break;

		case GetFileLogVersion:
			//
			//  Return version of the FSLog filter driver.  Verify
			//  we have a valid user buffer including valid
			//  alignment
			//
			if ((OutputBufferSize < sizeof(FLOGVER)) ||
				(OutputBuffer == NULL)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			//
			//  Validate Buffer alignment.  If a minifilter cares about
			//  the alignment value of the buffer pointer they must do
			//  this check themselves.  Note that a try/except will not
			//  capture alignment faults.
			//
			if (!IS_ALIGNED(OutputBuffer, sizeof(ULONG))) {
				status = STATUS_DATATYPE_MISALIGNMENT;
				break;
			}
			//
			//  Protect access to raw user-mode output buffer with an
			//  exception handler
			//
			try {
				((PFLOGVER)OutputBuffer)->Major = FSLOG_MAJ_VERSION;
				((PFLOGVER)OutputBuffer)->Minor = FSLOG_MIN_VERSION;
			} except(EXCEPTION_EXECUTE_HANDLER) {
				return GetExceptionCode();
			}
			*ReturnOutputBufferLength = sizeof(FLOGVER);
			status = STATUS_SUCCESS;
			break;

		case SetFileLogFilePath:
			wcscpy(logfilePath, (PWCHAR)(((PCOMMAND_MESSAGE)InputBuffer)->Data));
			status = STATUS_SUCCESS;
			break;

		case SetFileLogAttach:
			bAttach = (BOOLEAN)(((PCOMMAND_MESSAGE)InputBuffer)->Data[0]);
			//status = FslHookAllDrives(bAttach);
			break;

		default:
			status = STATUS_INVALID_PARAMETER;
			break;
		}

	}
	else {

		status = STATUS_INVALID_PARAMETER;
	}

	return status;
}




