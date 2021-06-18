#include "MMU.h"

void mmuTlbFlush(ArmMmu* mmu){
	
	UInt8 i, j;
	
	for(i = 0; i < MMU_TLB_BUCKET_NUM; i++){
		for(j = 0; j < MMU_TLB_BUCKET_SIZE; j++) mmu->tlb[i][j].sz = 0;
		mmu->replPos[i] = 0;
		mmu->readPos[i] = 0;
	}	
}


void mmuInit(ArmMmu* mmu, ArmMmuReadF readF, void* userData){

	__mem_zero(mmu, sizeof(ArmMmu));
	mmu->readF = readF;
	mmu->userData = userData;
	mmu->transTablPA = MMU_DISABLED_TTP;
	mmu->domainCfg = 0;
	mmuTlbFlush(mmu);
}

static _INLINE_ UInt8 mmuPrvHashAddr(UInt32 addr){	//addresses are granular on 1K 

	addr >>= 10;
	
	addr = addr ^ (addr >> 5) ^ (addr >> 10);
	
	return addr % MMU_TLB_BUCKET_NUM;
}

Boolean mmuTranslate(ArmMmu* mmu, UInt32 adr, Boolean priviledged, Boolean write, UInt32* paP, UInt8* fsrP){

	UInt32 va, pa = 0, sz, t;
	UInt8 dom, ap = 0;
	Boolean section = false, coarse = true, pxa_tex_page = false;
	UInt8 bucket;
	
	//handle the 'MMU off' case
		
	if(mmu->transTablPA == MMU_DISABLED_TTP){
		va = pa = 0;
		goto calc;
	}

	//check the TLB
	if(MMU_TLB_BUCKET_SIZE && MMU_TLB_BUCKET_NUM){
		
		bucket = mmuPrvHashAddr(adr);
				
		for(UInt8 j = 0, i = mmu->readPos[bucket]; j < MMU_TLB_BUCKET_SIZE; j++, i--){
			
			if(i == 0xFF) i = MMU_TLB_BUCKET_SIZE - 1;
			
			va = mmu->tlb[bucket][i].va;
			sz = mmu->tlb[bucket][i].sz;
			
			if(va <= adr && va + sz > adr){
				
				pa = mmu->tlb[bucket][i].pa;
				ap = mmu->tlb[bucket][i].ap;
				dom = mmu->tlb[bucket][i].domain;
				mmu->readPos[bucket] = i;
								
				goto check;
			}
		}
	}
	
	//read first level table
	
	if(mmu->transTablPA & 3){
		*fsrP = 0x01;	//alignment fault
		return false;
	}
	
	if(!mmu->readF(mmu->userData, &t, mmu->transTablPA + ((adr & 0xFFF00000) >> 18))){
		
		*fsrP = 0x0C;	//translation external abort first level
		return false;
	}
	
	dom = (t >> 5) & 0x0F;
	switch(t & 3){
		
		case 0:	//fault
		
			*fsrP = 0x5;	//section translation fault
			return false;
		
		case 1:	//coarse pagetable
			
			t &= 0xFFFFFC00UL;
			t += (adr & 0x000FF000UL) >> 10;
			break;
		
		case 2:	//1MB section
		
			pa = t & 0xFFF00000UL;
			va = adr & 0xFFF00000UL;
			sz = 1UL << 20;
			ap = (t >> 10) & 3;
			section = true;
			goto translated;
			
		case 3:	//fine page table
			
			coarse = false;
			t &= 0xFFFFF000UL;
			t += (adr & 0x000FFC00UL) >> 8;
			break;
	}
	
	
	//read second level table
	
	if(!mmu->readF(mmu->userData, &t, t)){
		*fsrP = 0x0E | (dom << 4);	//translation external abort second level
		return false;
	}
	
	switch(t & 3){
		
		case 0:	//fault
		
			*fsrP = 0x07 | (dom << 4);	//page translation fault
			return false;
		
		case 1:	//64K mapping
			
			pa = t & 0xFFFF0000UL;
			va = adr & 0xFFFF0000UL;
			sz = 65536UL;
			ap = (adr >> 14) & 3;		//in "ap" store which AP we need [of the 4]
			break;
		
		case 2:	//4K mapping (1K effective thenks to having 4 AP fields)
		
page_size_4k:
			pa = t & 0xFFFFF000UL;
			va = adr & 0xFFFFF000UL;
			sz = 4096;
			ap = (adr >> 10) & 3;		//in "ap" store which AP we need [of the 4]
			break;
			
		case 3:	//1K mapping
			
			if(coarse){
				
				pxa_tex_page = true;
				goto page_size_4k;	
			}
			
			pa = t & 0xFFFFFC00UL;
			va = adr & 0xFFFFFC00UL;
			ap = (t >> 4) & 3;		//in "ap" store the actual AP [and skip quarter-page resolution later using the goto]
			sz = 1024;
			goto translated;
	}
	
	
	//handle 4 AP sections

	UInt8 i = (t >> 4) & 0xFF;
	if(pxa_tex_page || ((i & 0x0F) == (i >> 4) && (i & 0x03) == ((i >> 2) & 0x03))){	//if all domains are the same, add the whole thing
		
		ap = (t >> 4) & 3;
	}
	else{	//take the quarter that is the one we need
	
		//err_str("quarter page found!\r\n");
		ap = (t >> (4 + 2 * ap)) & 3;
		sz /= 4;
		pa += ((UInt32)ap) * sz;
		va += ((UInt32)ap) * sz;
	}
	
	
translated:

	//insert tlb entry
	if(MMU_TLB_BUCKET_NUM && MMU_TLB_BUCKET_SIZE){
		
		mmu->tlb[bucket][mmu->replPos[bucket]].pa = pa;
		mmu->tlb[bucket][mmu->replPos[bucket]].sz = sz;
		mmu->tlb[bucket][mmu->replPos[bucket]].va = va;
		mmu->tlb[bucket][mmu->replPos[bucket]].ap = ap;
		mmu->tlb[bucket][mmu->replPos[bucket]].domain = dom;
		mmu->readPos[bucket] = mmu->replPos[bucket];
		if(++mmu->replPos[bucket] == MMU_TLB_BUCKET_SIZE) mmu->replPos[bucket] = 0;
	}

check:
				
	//check domain permissions
	
	switch((mmu->domainCfg >> (dom * 2)) & 3){
		case 2:	//RESERVED: unpredictable	(treat as no access)
			
			*fsrP = (section ? 0x08 : 0xB) | (dom << 4);	//section or page domain fault
			return false;
		
			
		case 3:	//MANAGER: allow all access
		
			goto calc;
		
	}

	//check permissions 
	
	switch(ap){
		
		case 0:
		
			if(write || (!mmu->R && (!priviledged || !mmu->S))) break;
		
		case 1:
			
			if(!priviledged) break;

		case 2:
			
			if(!priviledged && write) break;
	}

	goto calc;
	
//perm_err:

	*fsrP = (section ? 0x0D : 0x0F) | (dom << 4);		//section or subpage permission fault
	return false;
	
calc:

	*paP = adr - va + pa;
	return true;
}

UInt32 mmuGetTTP(ArmMmu* mmu){

	return mmu->transTablPA;
}

void mmuSetTTP(ArmMmu* mmu, UInt32 ttp){

	UInt8 i;
	
	mmuTlbFlush(mmu);
	for(i = 0; i < MMU_TLB_BUCKET_NUM; i++){
		
		mmu->replPos[i] = 0;
		mmu->readPos[i] = 0;
	}
	mmu->transTablPA = ttp;
}

void mmuSetS(ArmMmu* mmu, Boolean on){

	mmu->S = on;	
}

void mmuSetR(ArmMmu* mmu, Boolean on){

	mmu->R = on;	
}

Boolean mmuGetS(ArmMmu* mmu){
	
	return mmu->S;
}

Boolean mmuGetR(ArmMmu* mmu){
	
	return mmu->R;
}

UInt32 mmuGetDomainCfg(ArmMmu* mmu){
	
	return mmu->domainCfg;
}

void mmuSetDomainCfg(ArmMmu* mmu, UInt32 val){
	
	mmu->domainCfg = val;
}
