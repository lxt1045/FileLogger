#include "StringHashMap.h"
#include "HashFunction.h"
#include "FileLoggerData.h"
#include "stdlib.h"


#define AllocatePoolTag	'SHMP'

int __cdecl cmp(const void *a, const void *b)
{
	return *(ULONG *)a - *(ULONG *)b;  //强制转换类型
}//qsort(num, 100, sizeof(num[0]), cmp);

NTSTATUS StringHashInit(PSTRING_HASH_MAP pInitHashMap, PWCHAR initBuffer, ULONG initSize, ULONG maxCollision)
{
	//#pragma region #pragma endregion

	ULONG lStringCount = 0;

	pInitHashMap->pWcharPool = initBuffer;	//把传进来的字符串存好，当作hash table的底层缓存
	KdPrint(("initBuffer:%S", initBuffer));

	for (PWCHAR p = initBuffer; *p != L'\0'; ++p)//计算字符串数量
	{
		if (*p == L';')  ++lStringCount;
	}

	PUNICODE_STRING pUnicodeString = (PUNICODE_STRING)ExAllocatePoolWithTag(PagedPool,\
		lStringCount * sizeof(UNICODE_STRING), AllocatePoolTag);
#pragma region 把所有字符串解析到pUnicodeString
	{
		memset(pUnicodeString, 0, lStringCount * sizeof(UNICODE_STRING));
		ULONG stringIndex = 0;
		ULONG stringLength = 0;
		for (PWCHAR p = initBuffer; *p != L'\0'; ++p)
		{
			if (*p == L';')
			{
				pUnicodeString[stringIndex].Buffer = p - stringLength;
				if (*(p - 1) != L'\\')
				{
					*p = L'\\';
					++stringLength;
				}
				pUnicodeString[stringIndex].Length = stringLength * sizeof(WCHAR);
				pUnicodeString[stringIndex].MaximumLength = stringLength* sizeof(WCHAR);
				//KdPrint(("pUnicodeString[%d]:%wZ, Length:%d, MaximumLength:%d\n", i, &pUnicodeString[i], pUnicodeString[i].Length, pUnicodeString[i].MaximumLength));

				++stringIndex;
				stringLength = 0;
			}
			else
			{
				++stringLength;
				//KdPrint(("length:%d\n", length));
			}
		}
	}
#pragma endregion 
	for (int i = 0; i < lStringCount; ++i)
	{
		KdPrint(("pUnicodeString[%d]:%wZ, Length:%d, MaximumLength:%d\n", i, &pUnicodeString[i], pUnicodeString[i].Length, pUnicodeString[i].MaximumLength));
	}
	
	ULONG seed = 131;// 31 131 1313 13131 131313 etc..
	ULONG *pUnicodeStringHash = (PULONG)ExAllocatePoolWithTag(PagedPool, lStringCount * sizeof(ULONG), AllocatePoolTag);
#pragma region 把pUnicodeString里的所有字符串hash到pUnicodeStringHash中，并排序、检查碰撞，修改seed直到碰撞达到要求
	do
	{
		for (int j = 0; j < lStringCount; ++j)
		{
			pUnicodeStringHash[j] = BKDRHash(pUnicodeString[j].Buffer, pUnicodeString[j].Length / sizeof(WCHAR), seed);
			KdPrint(("pUnicodeStringHash[%d]:%u\n", j, pUnicodeStringHash[j]));
		}
		qsort(pUnicodeStringHash, lStringCount, sizeof(ULONG), cmp);
		int maxRepeatTimes = 1;
		for (int j = 1; j < lStringCount; ++j)//计算冲突数量
		{
			if (pUnicodeStringHash[j] == pUnicodeStringHash[j - 1])
			{
				++maxRepeatTimes;
			}
			else
			{
				if (maxRepeatTimes > maxCollision) break;
				else maxRepeatTimes = 1;
			}
		}
		if (maxRepeatTimes > maxCollision)
		{
			int lAdd = (seed / 10) % 10;
			seed = seed * 10 + lAdd;
			if (seed > 131313131)  break;
		}
		else
			break;
	} while (1);
#pragma endregion
	KdPrint(("seed:%u\n", seed));
	for (int i = 0; i < lStringCount; ++i)
	{
		KdPrint(("--->[%d]:%u\n", i, pUnicodeStringHash[i]));
	}
	
	ULONG lNullStringCount = 0;
	PULONG pTmpHashTableCount = NULL;
#pragma region 修改pInitHashMap->lHashTableSize，使HashTable碰撞满足要求，如果lStringCount<512:[1024~2048],其他: >2*lStringCount
	{
		if (lStringCount < 512)
			pInitHashMap->lHashTableSize = 1024;
		else
			pInitHashMap->lHashTableSize = lStringCount * 2;

		pTmpHashTableCount = (ULONG)ExAllocatePoolWithTag(PagedPool, pInitHashMap->lHashTableSize * 2 * sizeof(ULONG), AllocatePoolTag);
		memset(pTmpHashTableCount, 0, pInitHashMap->lHashTableSize * 2 * sizeof(ULONG));

		ULONG loop = 1, maxSize = pInitHashMap->lHashTableSize * 2;
		for (; (pInitHashMap->lHashTableSize < maxSize) && loop; ++pInitHashMap->lHashTableSize)
		{
			loop = 0;
			memset(pTmpHashTableCount, 0, pInitHashMap->lHashTableSize * sizeof(ULONG));
			for (int j = 0; j < lStringCount; j++)//如果有重复maxCollision个及以上的，重新选hash seed
			{
				if (++pTmpHashTableCount[pUnicodeStringHash[j] % pInitHashMap->lHashTableSize] > maxCollision)
				{
					loop = 1;
					break;
				}
			}
		}
		-- pInitHashMap->lHashTableSize;
		if (loop == 1)
		{
			memset(pTmpHashTableCount, 0, pInitHashMap->lHashTableSize * sizeof(ULONG));
			for (int j = 0; j < lStringCount; j++)//如果有重复maxCollision个及以上的，重新选hash seed
			{
				++pTmpHashTableCount[pUnicodeStringHash[j] % pInitHashMap->lHashTableSize];
			}
		}
	}
#pragma endregion
	ExFreePool(pUnicodeStringHash);

	KdPrint(("pInitHashMap->lHashTableSize:%u\n", pInitHashMap->lHashTableSize));
	for (int j = 0; j < pInitHashMap->lHashTableSize; j++)//统计String桶，做好相应的位置
	{
		KdPrint(("pTmpHashTable[%d]:%u\n", j, pTmpHashTableCount[j]));
		if (pTmpHashTableCount[j])   ++lNullStringCount;
	}
	
#pragma region 
	//开始分配真正的hash table 缓存
	pInitHashMap->ppHashTable = (PUNICODE_STRING *)ExAllocatePoolWithTag(PagedPool, \
										pInitHashMap->lHashTableSize * sizeof(PUNICODE_STRING), AllocatePoolTag);
	memset(pInitHashMap->ppHashTable, 0, pInitHashMap->lHashTableSize * sizeof(PUNICODE_STRING));//全部UNICODE_String初始化为0

	pInitHashMap->pStringPool = (PUNICODE_STRING)ExAllocatePoolWithTag(PagedPool, \
										(lStringCount + lNullStringCount) * sizeof(UNICODE_STRING), AllocatePoolTag);
	memset(pInitHashMap->pStringPool, 0, (lStringCount + lNullStringCount) * sizeof(UNICODE_STRING));//全部UNICODE_String初始化为0
	
	PUNICODE_STRING nextStringPointer = pInitHashMap->pStringPool;
	for (int j = 0; j < pInitHashMap->lHashTableSize; j++)//统计String桶，pInitHashMap->ppHashTable地址分配好
	{
		if (pTmpHashTableCount[j])
		{
			pInitHashMap->ppHashTable[j] = nextStringPointer;
			nextStringPointer = nextStringPointer + pTmpHashTableCount[j] + 1;
		}
		//KdPrint(("after:pTmpHashTable[%d]:%u\n", j, pTmpHashTableCount[j]));
	}

	for (int j = 0; j <lStringCount; ++j)
	{
		ULONG hashValue = BKDRHash(pUnicodeString[j].Buffer, pUnicodeString[j].Length / sizeof(WCHAR), seed);
		hashValue = hashValue % pInitHashMap->lHashTableSize;
		PUNICODE_STRING pString = pInitHashMap->ppHashTable[hashValue];
		if (pString == NULL) continue;
		while (pString->Length >0)  ++pString;
		pString->Buffer = pUnicodeString[j].Buffer;
		pString->Length = pUnicodeString[j].Length;
		pString->MaximumLength = pUnicodeString[j].MaximumLength;

		KdPrint(("pString:%d\n", pString - pInitHashMap->pStringPool));
	}
#pragma endregion
	ExFreePool(pUnicodeString);
	ExFreePool(pTmpHashTableCount);

	for (int j = 0; j < lStringCount + lNullStringCount; j++)//统计String桶，做好相应的位置
	{
		//这里有问题，缓存不对！！！！！
		KdPrint(("pInitHashMap->pStringPool[%d]:%wZ\n", j, &pInitHashMap->pStringPool[j]));
	}
	for (int j = 0; j < pInitHashMap->lHashTableSize; j++)//统计String桶，做好相应的位置
	{
		KdPrint(("pInitHashMap->ppHashTable[%d]:%wZ\n", j, pInitHashMap->ppHashTable[j]));
	}

	//*/
	return 1;
}
