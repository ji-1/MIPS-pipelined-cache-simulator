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

    Cache_Info = (Cache_Set_Info *)malloc(nset * sizeof(Cache_Set_Info*));

    for (i = 0; i < nset; i++) {
	Cache_Info[i].block = (Block_Info *)malloc(num_way * sizeof(Block_Info*));
    }

    for(i = 0; i < nset; i++) {
	for (j = 0; j < num_way; j++) {
	    //(Cache_Info[i].block[j]).valid=0;
	    (&(&Cache_Info[i])->block[i])->valid = 0;
	}
    }
    //for (i=0;i<4;i++)
//	printf("cache initialization: %d\n",Cache_Info[0].block[i].valid);
    for (i = 0; i < nset; i++) {
	Info_head[i] = NULL;
	Info_tail[i] = NULL;
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
    printf("cache read addr %x index %x tag %x offset %x\n",address,set_index, tag,offset);

    for (i = 0; i < 4; i++) {
	if (Cache_Info[set_index].block[i].valid && (Cache_Info[set_index].block[i].tag == tag)) {
	    // LRU
	    if (Cache_Info[set_index].block[i].valid && (Cache_Info[set_index].block[i].tag == tag)) {

		if (Info_head[set_index] != Info_tail[set_index]) {

		} else if (Info_head[set_index]!=&Cache_Info[set_index]) { 

		} else if (Info_tail[set_index] == &Cache_Info[set_index]) {

		    Block_Info *prev_block = &((&(&Cache_Info[set_index])->block[i])->prev);
		    prev_block->next = NULL;
		    (&(&Cache_Info[set_index])->block[i])->next = Info_head[set_index];
		    Block_Info *head_block = Info_head[set_index];
		    head_block->prev = &(Cache_Info[set_index]);
		    Info_head[set_index]=&(Cache_Info[set_index]);
		} else {
		    Block_Info *prev_block = &((&(&Cache_Info[set_index])->block[i])->prev);
		    Block_Info *next_block = (&(&Cache_Info[set_index])->block[i])->next;
		    prev_block->next = next_block;
		    next_block->prev = prev_block;

		    Block_Info *head_block = Info_head[set_index];
		    head_block->prev = &(Cache_Info[set_index]);
		    (&(&Cache_Info[set_index])->block[i])->next = Info_head[set_index];
		    Info_head[set_index]=&(Cache_Info[set_index]); 
		}
	    }
	    printf("cache read: %d\n",Cache[set_index][i][offset/BYTES_PER_WORD]);
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

    printf("cache lw miss %x block!\n", CURRENT_STATE.MEM_STALL_PC);
    for (i = 0; i < 4; i++){
	printf("lw valid: %d %d\n", i,Cache_Info[set_index].block[i].valid); 
	if (Cache_Info[set_index].block[i].valid == FALSE) {
	    printf("cache miss free block found!\n");

	    mem_read_block(CURRENT_STATE.MEM_STALL_PC, Cache[set_index][i]);

	    // LRU
	    if (Info_head[set_index]==NULL) {
		Info_head[set_index]=&(Cache_Info[set_index].block[i]);
		Info_tail[set_index]=&(Cache_Info[set_index].block[i]);
	    } else  {
		Block_Info *block = Info_head[set_index];
		(&(&Cache_Info[set_index])->block[i])->next = Info_head[set_index];
		block->prev = &(Cache_Info[set_index]);    
		Info_head[set_index]=&(Cache_Info[set_index]);
	    }

	    (&(&Cache_Info[set_index])->block[i])->valid = 1;
	printf("lw record way %d  valid %d\n", i,Cache_Info[set_index].block[i].valid); 
	    (&(&Cache_Info[set_index])->block[i])->tag = tag;
	    (&(&Cache_Info[set_index])->block[i])->dirty = 0;


	    return Cache[set_index][i][offset/BYTES_PER_WORD];
	}
    }
    // no free block left. LRU evict
}

void cache_write_32(uint32_t address, uint32_t value)
{
    int i;
    uint32_t set_index = (address >> 3) & 0x1;
    uint32_t tag = address >> 4;
    uint32_t offset = address & 0x7;
    printf("cache write address %x index %x tag %x offset %x\n",address, set_index, tag,offset);

    for (i = 0; i < 4; i++) {
	if (Cache_Info[set_index].block[i].valid && (Cache_Info[set_index].block[i].tag == tag)) {

	    if (Info_head[set_index] != Info_tail[set_index]) {

	    } else if (Info_head[set_index]!=&Cache_Info[set_index]) { 

	    } else if (Info_tail[set_index] == &Cache_Info[set_index]) {
		Block_Info *prev_block = &((&(&Cache_Info[set_index])->block[i])->prev);
		prev_block->next = NULL;
		(&(&Cache_Info[set_index])->block[i])->next = Info_head[set_index];
		Block_Info *head_block = Info_head[set_index];
		head_block->prev = &(Cache_Info[set_index]);
		Info_head[set_index]=&(Cache_Info[set_index]);
	    } else {
		Block_Info *prev_block = &((&(&Cache_Info[set_index])->block[i])->prev);
		Block_Info *next_block = (&(&Cache_Info[set_index])->block[i])->next;
		prev_block->next = next_block;
		next_block->prev = prev_block;

		Block_Info *head_block = Info_head[set_index];
		head_block->prev = &(Cache_Info[set_index]);
		(&(&Cache_Info[set_index])->block[i])->next = Info_head[set_index];
		Info_head[set_index]=&(Cache_Info[set_index]); 
	    }
	    Cache[set_index][i][offset/BYTES_PER_WORD]=value;
	    printf("cache write: %d\n",Cache[set_index][i][offset/BYTES_PER_WORD]);
	    (&(&Cache_Info[set_index])->block[i])->dirty = 1;
	    return;

	}
    }

    printf("sw cache X found\n");
    CURRENT_STATE.STALL_FOR_DCACHE=2;
    CURRENT_STATE.MEM_STALL_W_VALUE = value;
    CURRENT_STATE.MEM_STALL_PC = address; 
}

void cache_miss_mem_write_32(uint32_t address, uint32_t value) {

    int i;
    uint32_t set_index = (address >> 3) & 0x1;
    uint32_t tag = address >> 4;
    uint32_t offset = address & 0x7;

    printf("cache miss mem write index %x tag %x offset %x\n",set_index, tag,offset);
    for (i = 0; i < 4; i++){
	if (!Cache_Info[set_index].block[i].valid) {

	    mem_read_block(CURRENT_STATE.MEM_STALL_PC, Cache[set_index][i]);

	    // LRU
	    if (Info_head[set_index]==NULL) {
		Info_head[set_index]=&(Cache_Info[set_index].block[i]);
		Info_tail[set_index]=&(Cache_Info[set_index].block[i]);
	    } else  {
		Block_Info *block = Info_head[set_index];
		(&(&Cache_Info[set_index])->block[i])->next = Info_head[set_index];
		block->prev = &(Cache_Info[set_index]);    
		Info_head[set_index]=&(Cache_Info[set_index]);
	    }

	    (&(&Cache_Info[set_index])->block[i])->valid = 1;
	    (&(&Cache_Info[set_index])->block[i])->tag = tag;
	    (&(&Cache_Info[set_index])->block[i])->dirty = 1;

	    Cache[set_index][i][offset/BYTES_PER_WORD]=value;
	}	
    }
}

