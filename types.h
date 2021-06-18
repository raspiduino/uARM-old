#ifndef _TYPES_H_
#define _TYPES_H_


typedef unsigned long UInt32;
typedef signed long Int32;
typedef unsigned short UInt16;
typedef signed short Int16;
typedef unsigned char UInt8;
typedef signed char Int8;
typedef unsigned char Err;
typedef unsigned char Boolean;

#define true	1
#define false	0

#ifndef NULL
	#define NULL ((void*)0)
#endif

//#define TYPE_CHECK ((sizeof(UInt32) == 4) && (sizeof(UInt16) == 2) && (sizeof(UInt8) == 1))

#define errNone		0x00
#define errInternal	0x01


#define _INLINE_   	inline __attribute__ ((always_inline))
#define _UNUSED_	__attribute__((unused))

#define memset __memset_disabled__
#define memcpy __memcpy_disabled__

#endif

