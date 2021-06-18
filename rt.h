#include "types.h"

/* runtime stuffs */
void err_str(const char* str);
void err_hex(UInt32 val);
void err_dec(UInt32 val);
void __mem_zero(UInt8* ptr, UInt16 sz);
UInt32 rtcCurTime(void);
void* emu_alloc(UInt32 size);
void emu_free(void* ptr);
void __mem_copy(UInt8* d_, const UInt8* s_, UInt32 sz);