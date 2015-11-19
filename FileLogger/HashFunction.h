#pragma once
#include <fltKernel.h>


// BKDR Hash Function
/*
unsigned int BKDRHash(char *str)
{
unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
unsigned int hash = 0;

while (*str)
{
hash = hash * seed + (*str++);
}

return (hash & 0x7FFFFFFF);
}*/
ULONG BKDRHash(WCHAR *str, ULONG lenth, ULONG seed);

unsigned int SDBMHash(char *str);

// RS Hash Function
unsigned int RSHash(char *str);

// JS Hash Function
unsigned int JSHash(char *str);

// P. J. Weinberger Hash Function
unsigned int PJWHash(char *str);

// ELF Hash Function
unsigned int ELFHash(char *str);

// DJB Hash Function
unsigned int DJBHash(char *str);

// AP Hash Function
unsigned int APHash(char *str);