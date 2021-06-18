#include "mem.h"



void memInit(ArmMem* mem){

	for(UInt8 i = 0; i < MAX_MEM_REGIONS; i++){
		mem->regions[i].sz = 0;
	}
}

Boolean memRegionAdd(ArmMem* mem, UInt32 pa, UInt32 sz, ArmMemAccessF aF, void* uD){
	
	//check for intersection with another region
	
	for(UInt8 i = 0; i < MAX_MEM_REGIONS; i++){
		
		if(!mem->regions[i].sz) continue;
		if((mem->regions[i].pa <= pa && mem->regions[i].pa + mem->regions[i].sz > pa) || (pa <= mem->regions[i].pa && pa + sz > mem->regions[i].pa)){
		
			return false;		//intersection -> fail
		}
	}
	
	
	//find a free region and put it there
	
	for(UInt8 i = 0; i < MAX_MEM_REGIONS; i++){
		if(mem->regions[i].sz == 0){
		
			mem->regions[i].pa = pa;
			mem->regions[i].sz = sz;
			mem->regions[i].aF = aF;
			mem->regions[i].uD = uD;
		
			return true;
		}
	}
	
	
	//fail miserably
	
	return false;	
}

Boolean memRegionDel(ArmMem* mem, UInt32 pa, UInt32 sz){
	
	for(UInt8 i = 0; i < MAX_MEM_REGIONS; i++){
		if(mem->regions[i].pa == pa && mem->regions[i].sz ==sz){
		
			mem->regions[i].sz = 0;
			return true;
		}
	}
	
	return false;
}

Boolean memAccess(ArmMem* mem, UInt32 addr, UInt8 size, Boolean write, void* buf){
	
	for(UInt8 i = 0; i < MAX_MEM_REGIONS; i++){
		if(mem->regions[i].pa <= addr && mem->regions[i].pa + mem->regions[i].sz > addr){
		
			return mem->regions[i].aF(mem->regions[i].uD, addr, size, write & 0x7F, buf);
		}
	}
	
	return false; // If failed
}

