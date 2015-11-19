#pragma once
#include <fltKernel.h>

#define HASH_MAP_CAN_INC 
#define VALUE_COUNT  1024

typedef struct _HASH_MAP_VALUE{
	struct _HASH_MAP_VALUE *pNextNode; //write:InterlockedExchange( &nextIndex, newNextIndex ),read:1,get;2,InterlockedCompareExchange

	ULONG value;//processId
	ULONG IsFilter;//是否要过滤掉（在pre中如果过滤掉，则，把上下文设为NULL，在pro中看到NULL的上下文则，直接退出）

	PWCHAR info;//"[ProcessName\0][UserName\0]"
	USHORT processNameLenth;
	USHORT userNameLenth;

	//KSPIN_LOCK changeLock;;//KIRQL oldIrql; KeAcquireSpinLock(&OutputBufferLock, &oldIrql);	KeReleaseSpinLock(&OutputBufferLock, oldIrql);
}HASH_MAP_VALUE, *PHASH_MAP_VALUE;

typedef struct _HASH_MAP_POOL{
	struct _HASH_MAP_POOL *pNextNode;
	ULONG poolSize;
	HASH_MAP_VALUE poolArray[1];
}HASH_MAP_POOL, *PHASH_MAP_POOL;

typedef struct _HASH_MAP {
	PHASH_MAP_VALUE valueArray[VALUE_COUNT];

	__volatile LONG threadCount;//InterlockedIncrement,InterlockedDecrement,InterlockedExchangeAdd
								//用于退出时确认没有进程使用后，方能删除缓冲区
	PHASH_MAP_POOL pValuePool;//emery pool; index:0~max ; index==-1:NULL;InterlockedExchangePointer
	__volatile PHASH_MAP_VALUE pFreeList;//InterlockedIncrement,InterlockedDecrement,InterlockedExchangeAdd

	KSPIN_LOCK hashMapModifyLock;//KIRQL oldIrql; KeAcquireSpinLock(&OutputBufferLock, &oldIrql);	KeReleaseSpinLock(&OutputBufferLock, oldIrql);
}HASH_MAP, *PHASH_MAP;

VOID keSleepMsec(LONG msec);
PHASH_MAP getHashMapInstance();//采用工厂模式?
NTSTATUS hashFun();
NTSTATUS hashInit(PHASH_MAP pInitHashMap, ULONG initSize);
NTSTATUS hashInsert(PHASH_MAP pHashMap, ULONG processId, PUNICODE_STRING strProcessName, PUNICODE_STRING strUserName);
NTSTATUS hashRemove(PHASH_MAP pHashMap, ULONG processId);
NTSTATUS hashDestroy(PHASH_MAP pHashMap);
VOID printfHashMap(PHASH_MAP pInitHashMap);
