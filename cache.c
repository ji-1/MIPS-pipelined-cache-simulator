/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   cache.c                                                   */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

//#include "cache.h"
#include "util.h"

/* cache.c : Implement your functions declared in cache.h */


/***************************************************************/
/*                                                             */
/* Procedure: setupCache                                       */
/*                                                             */
/* Purpose: Allocates memory for your cache                    */
/*                                                             */
/***************************************************************/

void setupCache(int capacity, int num_way, int block_size)
{
    /*	code for initializing and setting up your cache	*/
    /*	You may add additional code if you need to	*/

    int i,j; //counter
    int nset=0; // number of sets
    int _wpb=0; //words per block

    nset = capacity/(block_size*num_way);
    _wpb = block_size/BYTES_PER_WORD;
    Cache = (uint32_t***)malloc(nset*sizeof(uint32_t**));

    for (i = 0; i < nset; i++) {
	Cache[i] = (uint32_t**)malloc(num_way * sizeof(uint32_t*));
    }

    for (i = 0; i < nset; i++){
	for (j = 0; j < num_way; j++){
	    Cache[i][j] = (uint32_t*)malloc(sizeof(uint32_t) * (_wpb));
	}
    }

    Cache_Info = (Cache_Set_Info *)malloc(nset * sizeof(Cache_Set_Info));

    for (i = 0; i < nset; i++) {
	Cache_Info[i].block = (Block_Info *)malloc(num_way * sizeof(Block_Info));
    }

    for(i = 0; i < nset; i++) {
	for (j = 0; j < num_way; j++) {
	    (Cache_Info[i].block[j]).valid=0;
	    //	    (&(&Cache_Info[i])->block[i])->valid = 0;
	}
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: setCacheMissPenalty                  	       */
/*                                                             */
/* Purpose: Sets how many cycles your pipline will stall       */
/*                                                             */
/***************************************************************/

void setCacheMissPenalty(int penalty_cycles)
{
    /*	code for setting up miss penalty			*/
    /*	You may add additional code if you need to	*/
    miss_penalty = penalty_cycles;
}

/* Please declare and implement additional functions for your cache */
uint32_t cache_read_32(uint32_t address)
{

    int i;
    uint32_t set_index = (address >> 3) & 0x1;
    uint32_t tag = address >> 4;
    uint32_t offset = address & 0x7;

    for (i = 0; i < 4; i++) {
	if (Cache_Info[set_index].block[i].valid && (Cache_Info[set_index].block[i].tag == tag)) {
	    // LRU
	    (Cache_Info[set_index].block[i]).time=timer;
	    return Cache[set_index][i][offset/BYTES_PER_WORD];
	}
    }
    CURRENT_STATE.STALL_FOR_DCACHE=1;
    CURRENT_STATE.MEM_STALL_PC = address; 
    return 0;
}

uint32_t cache_miss_mem_read_32()
{
    int i;
    uint32_t set_index = (CURRENT_STATE.MEM_STALL_PC >> 3) & 0x1;
    uint32_t tag = CURRENT_STATE.MEM_STALL_PC >> 4;
    uint32_t offset = CURRENT_STATE.MEM_STALL_PC & 0x7;

    for (i = 0; i < 4; i++){
	if (Cache_Info[set_index].block[i].valid == FALSE) {
	    mem_read_block(CURRENT_STATE.MEM_STALL_PC, Cache[set_index][i]);
	    // LRU
	    (Cache_Info[set_index].block[i]).addr=CURRENT_STATE.MEM_STALL_PC;
	    (Cache_Info[set_index].block[i]).time=timer;
	    (Cache_Info[set_index].block[i]).valid=1;
	    (Cache_Info[set_index].block[i]).tag=tag;
	    (Cache_Info[set_index].block[i]).dirty=0;

	    return Cache[set_index][i][offset/BYTES_PER_WORD];
	}
    }
    // no free block left. LRU evict
    int victim = 0;
    int victim_t=Cache_Info[set_index].block[0].time;
    for (i=1;i<4;i++) {
	if (victim_t > (Cache_Info[set_index].block[i]).time) {
	    victim_t = (Cache_Info[set_index].block[i]).time;
	    victim=i;
	}
    }

    if ((Cache_Info[set_index].block[victim]).dirty == 1) {
	// mem write
	mem_write_block((Cache_Info[set_index].block[victim]).addr, Cache[set_index][victim]);
    }

    (Cache_Info[set_index].block[victim]).valid=0;
    return cache_miss_mem_read_32();
}

void cache_write_32(uint32_t address, uint32_t value)
{
    int i;
    uint32_t set_index = (address >> 3) & 0x1;
    uint32_t tag = address >> 4;
    uint32_t offset = address & 0x7;

    for (i = 0; i < 4; i++) {
	if (Cache_Info[set_index].block[i].valid && (Cache_Info[set_index].block[i].tag == tag)) {

	    (Cache_Info[set_index].block[i]).time=timer;
	    Cache[set_index][i][offset/BYTES_PER_WORD]=value;
	    (Cache_Info[set_index].block[i]).dirty=1;
	    return;

	}
    }

    CURRENT_STATE.STALL_FOR_DCACHE=2;
    CURRENT_STATE.MEM_STALL_W_VALUE = value;
    CURRENT_STATE.MEM_STALL_PC = address; 
}

void cache_miss_mem_write_32(uint32_t address, uint32_t value) {

    int i;
    uint32_t set_index = (address >> 3) & 0x1;
    uint32_t tag = address >> 4;
    uint32_t offset = address & 0x7;

    for (i = 0; i < 4; i++){
	if (!Cache_Info[set_index].block[i].valid) {

	    mem_read_block(CURRENT_STATE.MEM_STALL_PC, Cache[set_index][i]);

	    // LRU
	    (Cache_Info[set_index].block[i]).time=timer;
	    (Cache_Info[set_index].block[i]).valid=1;
	    (Cache_Info[set_index].block[i]).addr=CURRENT_STATE.MEM_STALL_PC;
	    (Cache_Info[set_index].block[i]).tag=tag;
	    (Cache_Info[set_index].block[i]).dirty=1;

	    Cache[set_index][i][offset/BYTES_PER_WORD]=value;
	    return;
	}	
    }
    // LRU
    int victim = 0;
    int victim_t=Cache_Info[set_index].block[0].time;
    for (i=1;i<4;i++) {
	if (victim_t > (Cache_Info[set_index].block[i]).time) {
	    victim_t = (Cache_Info[set_index].block[i]).time;
	    victim=i;
	}
    }
    if ((Cache_Info[set_index].block[victim]).dirty == 1) {
	// mem write
	mem_write_block((Cache_Info[set_index].block[victim]).addr, Cache[set_index][victim]);
    }
    (Cache_Info[set_index].block[victim]).valid=0;
    return cache_miss_mem_write_32(address, value);

}

void cache_flush() {
    int i,j;
    for (i=0;i<2;i++) {
	for (j=0;j<4;j++) {
	    if ((Cache_Info[i].block[j]).dirty == 1) {
		mem_write_block((Cache_Info[i].block[j]).addr, Cache[i][j]);
	    }
	}
    }
}
