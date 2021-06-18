#include "types.h"
#include "CPU.h"
#include "icache.h"

void icacheInval(icache* ic){
	
	for(UInt8 i = 0; i < ICACHE_BUCKET_NUM; i++){
		for(UInt8 j = 0; j < ICACHE_BUCKET_SZ; j++) ic->lines[i][j].info = 0;
		ic->ptr[i] = 0;
	}
}

void icacheInit(icache* ic, ArmCpu* cpu, ArmCpuMemF memF){

	ic->cpu = cpu;
	ic->memF = memF;
	
	icacheInval(ic);	
}


static UInt8 icachePrvHash(UInt32 addr){

	addr >>= ICACHE_L;
	addr &= (1UL << ICACHE_S) - 1UL;

	return addr;
}

void icacheInvalAddr(icache* ic, UInt32 va){

	Int8 bucket;
	icacheLine* lines;
	
	va -= va % ICACHE_LINE_SZ;

	bucket = icachePrvHash(va);
	lines = ic->lines[bucket];
	
	for(Int8 i = 0, j = ic->ptr[bucket]; (UInt8)i < ICACHE_BUCKET_SZ; i++){
		
		if(--j == -1) j = ICACHE_BUCKET_SZ - 1;
		
		if((lines[j].info & (ICACHE_ADDR_MASK | ICACHE_USED_MASK)) == (va | ICACHE_USED_MASK)){	//found it!
		
			lines[j].info = 0;
		}
	}
}

/*
	we cannot have data overlap cachelines since data is self aligned (word on 4-byte boundary, halfwords on2, etc. this is enforced elsewhere
*/

Boolean icacheFetch(icache* ic, UInt32 va, UInt8 sz, Boolean priviledged, UInt8* fsrP, void* buf){

	UInt32 off = va % ICACHE_LINE_SZ;
	Int8 i, j, bucket;
	icacheLine* lines;
	icacheLine* line;
	
	va -= off;

	bucket = icachePrvHash(va);
	lines = ic->lines[bucket];
	
	for(i = 0, j = ic->ptr[bucket]; (UInt8)i < ICACHE_BUCKET_SZ; i++){
		
		if(--j == -1) j = ICACHE_BUCKET_SZ - 1;
		
		if((lines[j].info & (ICACHE_ADDR_MASK | ICACHE_USED_MASK)) == (va | ICACHE_USED_MASK)){	//found it!
		
			if(sz == 4){
				*(UInt32*)buf = *(UInt32*)(lines[j].data + off);
			}
			else if(sz == 2){
				*(UInt16*)buf = *(UInt16*)(lines[j].data + off);
			}
			else __mem_copy(buf, lines[j].data + off, sz);
			return priviledged || !(lines[j].info & ICACHE_PRIV_MASK);	
		}
	}
	//if we're here, we found nothing - time to populate the cache
	j = ic->ptr[bucket]++;
	if(ic->ptr[bucket] == ICACHE_BUCKET_SZ) ic->ptr[bucket] = 0;
	line = lines + j;
	
	line->info = va | (priviledged ? ICACHE_PRIV_MASK : 0);
	if(!ic->memF(ic->cpu, line->data, va, ICACHE_LINE_SZ, false, priviledged, fsrP)){
	
		return false;	
	}
	line->info |= ICACHE_USED_MASK;
	
	if(sz == 4){
		*(UInt32*)buf = *(UInt32*)(line->data + off);
	}
	else if(sz == 2){
		*(UInt16*)buf = *(UInt16*)(line->data + off);
	}
	else __mem_copy(buf, line->data + off, sz);
	return true;
}

