#ifndef DENIS_META_H_
#define DENIS_META_H_

#include "stdint.h"

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

//NOTE(denis): assumes the char array is a valid string
static bool charInArray(char c, char array[])
{
    bool result = false;

    if (array != 0)
    {
	for (int i = 0; array[i] != 0 && !result; ++i)
	{
	    if (array[i] == c)
		result = true;
	}
    }
    
    return result;
}

//NOTE(denis): only works for valid strings
static bool stringsEqual(char *A, char *B)
{
    bool result = true;

    int charIndex = 0;
    while (A[charIndex] != 0 && B[charIndex] != 0 && result)
    {
	if (A[charIndex] != B[charIndex])
	    result = false;
	
	++charIndex;
    }

    if (A[charIndex] != B[charIndex])
	result = false;
    
    return result;
}

#endif
