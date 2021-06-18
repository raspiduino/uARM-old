#include "rt.h"

void err_hex(UInt32 val){

	char x[9];
	unsigned char c;
	
	x[8] = 0;
	
	for(unsigned char i = 0; i < 8; i++){
		
		c = val & 0x0F;
		val >>= 4;
		c = (c >= 10) ? (c + 'A' - 10) : (c + '0');
		x[7 - i] = c;	
	}
	
	err_str(x);
}

void err_dec(UInt32 val){
	
	char x[16];
	unsigned char i, c;
	
	x[sizeof(x) - 1] = 0;
	
	for(i = 0; i < sizeof(x) - 1; i++){
		
		c = (val % 10) + '0';
		val /= 10;
		x[sizeof(x) - 2 - i] = c;	
		if(!val) break;
	}
	err_str(x + sizeof(x) - 2 - i);
}

void __mem_zero(UInt8* ptr, UInt16 sz){
	
	UInt8* p = ptr;
	
	while(sz--) *p++ = 0;	
}

void __mem_copy(UInt8* d_, const UInt8* s_, UInt32 sz){
	
	UInt8* d = d_;
	const UInt8* s = s_;
	
	while(sz--) *d++ = *s++;
}