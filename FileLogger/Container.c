#include "Container.h"
#include "FileLoggerData.h"

#define hashFun(x) (((x)/4)%VALUE_COUNT)

#define AllocatePoolTag	'FLCN'

NTSTATUS hashInsert(PHASH_MAP pHashMap, ULONG processId, PUNICODE_STRING strProcessName, PUNICODE_STRING strUserName)
{
	PAGED_CODE();

	InterlockedIncrement(&pHashMap->threadCount);
	if (pHashMap->threadCount <= 0)//when destroy!
		goto ERROR_EXIT;

	KIRQL oldIrql;
	KeAcquireSpinLock(&pHashMap->hashMapModifyLock, &oldIrql);

	PHASH_MAP_VALUE *ppValue = pHashMap->valueArray + hashFun(processId);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("                             valueArray[%d]==> value:%d\n", \
		ppValue - pHashMap->valueArray, processId));

	while (*ppValue != NULL)
	{
		ppValue = &(*ppValue)->pNextNode;
	}

	if (pHashMap->pFreeList == NULL)//new valuePool
	{
		HASH_MAP_POOL *oldValuePool = NULL;
		//pHashMap->poolSize *= 2;
		HASH_MAP_POOL *newValuePool = ExAllocatePoolWithTag(PagedPool, pHashMap->pValuePool->poolSize * sizeof(HASH_MAP_VALUE) \
			+ sizeof(HASH_MAP_POOL), AllocatePoolTag);
		if (newValuePool == NULL)
		{
			goto ERROR_EXIT;
		}
		for (oldValuePool = pHashMap->pValuePool; oldValuePool->pNextNode != NULL; oldValuePool = oldValuePool->pNextNode);
		oldValuePool->pNextNode = newValuePool;
		newValuePool->poolSize = pHashMap->pValuePool->poolSize;
		newValuePool->pNextNode = NULL;

		pHashMap->pFreeList = newValuePool->poolArray;
		for (ULONG i = 0; i < newValuePool->poolSize; i++)
		{
			newValuePool->poolArray[i].value = -1;
			newValuePool->poolArray[i].pNextNode = &newValuePool->poolArray[i + 1];
			newValuePool->poolArray[i].info = NULL;
			newValuePool->poolArray[i].processNameLenth = 0;
			newValuePool->poolArray[i].userNameLenth = 0;
		}
		newValuePool->poolArray[newValuePool->poolSize - 1].pNextNode = NULL;//链表必须以NULL结尾，否则无法确定是否结束，造成越界		
	}
	HASH_MAP_VALUE *pNewValue = pHashMap->pFreeList;
	pNewValue->info = ExAllocatePoolWithTag(PagedPool, strProcessName->Length \
											+ strUserName->Length + 2 * sizeof(WCHAR), AllocatePoolTag);
	if (pNewValue->info == NULL)
		goto ERROR_EXIT;
	InterlockedExchange(&pHashMap->pFreeList, pNewValue->pNextNode);

	pNewValue->pNextNode = NULL;
	pNewValue->value = processId;
	pNewValue->processNameLenth = strProcessName->Length /sizeof(WCHAR);
	pNewValue->userNameLenth = strUserName->Length / sizeof(WCHAR);

	//注意避免前面出错后，lenth出现奇数的情况，导致memcpy越界蓝屏
	memcpy(pNewValue->info, strProcessName->Buffer, pNewValue->processNameLenth*sizeof(WCHAR));
	pNewValue->info[pNewValue->processNameLenth] = L'\0';

	memcpy(pNewValue->info + pNewValue->processNameLenth + 1, strUserName->Buffer, pNewValue->userNameLenth*sizeof(WCHAR));
	pNewValue->info[pNewValue->processNameLenth + 1 + pNewValue->userNameLenth] = L'\0';

	*ppValue = pNewValue;
	//printfHashMap(pHashMap);
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Insert:PHASH_MAP->pValuePool->poolArray[%d]==> value:%d, processName:%S, userName:%S\n", \
			pNewValue - pHashMap->pValuePool->poolArray, processId, \
			pNewValue->info, pNewValue->info + pNewValue->processNameLenth + 1));
	}

	KeReleaseSpinLock(&pHashMap->hashMapModifyLock, oldIrql);
	InterlockedDecrement(&pHashMap->threadCount);
	return 1;

ERROR_EXIT:
	KeReleaseSpinLock(&pHashMap->hashMapModifyLock, oldIrql);
	InterlockedDecrement(&pHashMap->threadCount);
	return -1;
}

NTSTATUS hashRemove(PHASH_MAP pHashMap, ULONG processId)
{
	PAGED_CODE();

	InterlockedIncrement(&pHashMap->threadCount);
	if (pHashMap->threadCount <= 0)//when destroy!
	{
		InterlockedDecrement(&pHashMap->threadCount);
		return -1;
	}

	KIRQL oldIrql;
	KeAcquireSpinLock(&pHashMap->hashMapModifyLock, &oldIrql);

	PHASH_MAP_VALUE *ppValue = pHashMap->valueArray + hashFun(processId);
	
	while (*ppValue != NULL && (*ppValue)->value != processId)
	{
		ppValue = &(*ppValue)->pNextNode;
	}
	if (*ppValue == NULL)
		goto ERROR_EXIT;

	PHASH_MAP_VALUE pValue = *ppValue;
	if (pValue->info != NULL)
	{
		PHASH_MAP_VALUE  pOldValue = InterlockedExchangePointer(ppValue, (*ppValue)->pNextNode);

		if (pOldValue != NULL)
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Remove:PHASH_MAP->pValuePool->poolArray[%d]==> value:%d, processName:%S, userName:%S,processNameLenth:%d,userNameLenth:%d\n", \
				pOldValue - pHashMap->pValuePool->poolArray, pOldValue->value, pOldValue->info,\
				pOldValue->info + pOldValue->processNameLenth + 1, \
				pOldValue->processNameLenth, pOldValue->userNameLenth));

			ExFreePool(pOldValue->info);
			pOldValue->info = NULL;
			pOldValue->pNextNode = pHashMap->pFreeList;
			pHashMap->pFreeList = pOldValue;
		}
	}

	KeReleaseSpinLock(&pHashMap->hashMapModifyLock, oldIrql);
	InterlockedDecrement(&pHashMap->threadCount);
	return 1;

ERROR_EXIT:
	KeReleaseSpinLock(&pHashMap->hashMapModifyLock, oldIrql);
	InterlockedDecrement(&pHashMap->threadCount);
	return -1;
}


NTSTATUS hashInit(PHASH_MAP pInitHashMap, ULONG initSize)
{
	PAGED_CODE();

	//KeInitializeSpinLock(&pInitHashMap->hashMapModifyLock);
	InitFilterRules();

	if (pInitHashMap == NULL)
		return -1;

	//pInitHashMap->threadCount = 0;
	InterlockedExchange(&pInitHashMap->threadCount, 0);
	for (ULONG i = 0; i < sizeof(pInitHashMap->valueArray) / sizeof(pInitHashMap->valueArray[0]); i++)
	{
		pInitHashMap->valueArray[i] = NULL;
		//InterlockedExchange((ULONG *)(pInitHashMap->valueArray + i), -1);
	}

	PHASH_MAP_POOL pPool = ExAllocatePoolWithTag(PagedPool, initSize * sizeof(HASH_MAP_VALUE) + sizeof(HASH_MAP_POOL), AllocatePoolTag);
	if (pPool == NULL)
	{
		ExFreePool(pPool);
		return -1;
	}
	else
	{
		pInitHashMap->pValuePool = pPool;
		pPool->poolSize = initSize;
		pInitHashMap->pFreeList = pPool->poolArray;
		pPool->pNextNode = NULL;
		for (ULONG i = 0; i < pPool->poolSize; i++)
		{
			pPool->poolArray[i].value = -1;
			pPool->poolArray[i].pNextNode = &pPool->poolArray[i + 1];
			pPool->poolArray[i].info = NULL;
			pPool->poolArray[i].processNameLenth = 0;
			pPool->poolArray[i].userNameLenth = 0;
		}
		pPool->poolArray[pPool->poolSize - 1].pNextNode = NULL;
	}
	return 1;
}

NTSTATUS hashDestroy(PHASH_MAP pHashMap)
{

	PAGED_CODE();

	InterlockedIncrement(&pHashMap->threadCount);
	if (pHashMap->threadCount <= 0)//when destroy!
	{
		InterlockedDecrement(&pHashMap->threadCount);
		return -1;
	}
	LONG threadCount = InterlockedExchange(&pHashMap->threadCount, -1);
	while (threadCount + pHashMap->threadCount > 0)// threadCount + pHashMap->threadCount != 0
	{
		keSleepMsec(1000);
	}
	for (PHASH_MAP_POOL pValuePool = pHashMap->pValuePool; pValuePool != NULL;)
	{
		PHASH_MAP_POOL pPool;
		for (ULONG i = 0; i < pValuePool->poolSize; i++)
		{
			if (pValuePool->poolArray[i].info != NULL)
			{
				ExFreePool(pValuePool->poolArray[i].info);
				pValuePool->poolArray[i].info = NULL;
			}
		}
		pPool = pValuePool;
		pValuePool = pValuePool->pNextNode;
		ExFreePool(pPool);
	}
	pHashMap->pValuePool = NULL;

	InterlockedDecrement(&pHashMap->threadCount);
	return 1;
}

#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)
VOID keSleepMsec(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, 0, &my_interval);
}

VOID printfHashMap(PHASH_MAP pHashMap)
{
	PAGED_CODE(); 
	InterlockedIncrement(&pHashMap->threadCount);
	if (pHashMap->threadCount <= 0)//when destroy!
	{
		InterlockedDecrement(&pHashMap->threadCount);
		return -1;
	}

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->threadCount:%d\n", pHashMap->threadCount));
	/*for (ULONG i = 0; i < sizeof(pHashMap->valueArray) / sizeof(pHashMap->valueArray[0]); i++)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->valueArray[%d]:%ulld\n", i, pHashMap->valueArray[i]));
	}*/
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->pFreeList:%ulld\n", pHashMap->pFreeList));
	if (pHashMap->pValuePool == NULL)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->valuePool:NULL\n"));
	}
	else 
	{
		for (PHASH_MAP_POOL pValuePool = pHashMap->pValuePool; pValuePool != NULL; pValuePool = pValuePool->pNextNode)
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->poolSize:%d\n", pValuePool->poolSize));
			for (ULONG i = 0; i < pValuePool->poolSize; i++)
			{
				HASH_MAP_VALUE *pValue = &pValuePool->poolArray[i];
				if (pValue->info != NULL)
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->valueArray[%d]==> value:%d, processName:%S, userName:%S, nextNode:%ulld\n",\
						i, pValue->value, pValue->info, pValue->info + pValue->processNameLenth + 1, pValue->pNextNode\
						));
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("PHASH_MAP->valueArray[%d]==> value:%d, pValue->info == NULL, nextNode:%ulld",\
						i, pValue->value, pValue->pNextNode\
						));
				}
			}
		}
	}
	InterlockedDecrement(&pHashMap->threadCount);
}


