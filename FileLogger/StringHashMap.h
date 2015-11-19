/*
******************************
*		本hash表用于存储字符串，不能插入只能初始化
*	在初始化的时候，需检查碰撞，不能达到要求则需更改hash函数的参数和更改table大小,采用BKDRHash函数
*	ULONG BKDRHash(WCHAR *str, ULONG lenth, ULONG seed) //ULONG seed = 31 131 1313 13131 131313 etc..
*	如果所有seed都不能满足，则选择碰撞最小的seed！
*
******************************
*/

#pragma once
#include <fltKernel.h>


#define VALUE_COUNT  1024

typedef struct _STRING_HASH_MAP {
	PUNICODE_STRING *ppHashTable;
	ULONG lHashTableSize;

	ULONG IsIncludeFilter;//是白名单还是黑名单（在pre中如果过滤掉，则，把上下文设为NULL，在pro中看到NULL的上下文则，直接退出）
									//InterlockedIncrement,InterlockedDecrement,InterlockedExchangeAdd
	__volatile LONG threadCount;//用于退出时确认没有进程使用后，方能删除缓冲区；

	PUNICODE_STRING pStringPool;//emery pool; index:0~max ; index==-1:NULL;InterlockedExchangePointer
	PWCHAR pWcharPool;//emery pool; index:0~max ; index==-1:NULL;InterlockedExchangePointer
}STRING_HASH_MAP, *PSTRING_HASH_MAP;


PSTRING_HASH_MAP getStringHashMapInstance();//采用工厂模式?
NTSTATUS StringHashInit(PSTRING_HASH_MAP pInitHashMap, PWCHAR initBuffer, ULONG initSize, ULONG maxCollision);
NTSTATUS StringHashInsert(PSTRING_HASH_MAP pHashMap, ULONG processId, PUNICODE_STRING strProcessName, PUNICODE_STRING strUserName);
NTSTATUS StringHashRemove(PSTRING_HASH_MAP pHashMap, ULONG processId);
NTSTATUS StringHashDestroy(PSTRING_HASH_MAP pHashMap);
VOID printfStringHashMap(PSTRING_HASH_MAP pInitHashMap);
