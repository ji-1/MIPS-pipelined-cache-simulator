/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   cache.c                                                   */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

#include "cache.h"
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
	nset=capacity/(block_size*num_way);
	_wpb = block_size/BYTES_PER_WORD;

	Cache = (uint32_t***)malloc(nset*sizeof(uint32_t**));
	
    for (i=0;i<nset;i++) {
        Cache[i] = (uint32_t**)malloc(num_way*sizeof(uint32_t*));
    }
    
    for (i=0; i<nset; i++){	
        for (j=0; j<num_way; j++){
            Cache[i][j]=(uint32_t*)malloc(sizeof(uint32_t)*(_wpb));
        }
	}
}


/***************************************************************/
/*                                                             */
/* Procedure: setCacheMissPenalty                  	           */
/*                                                             */
/* Purpose: Sets how many cycles your pipline will stall       */
/*                                                             */
/***************************************************************/

void setCacheMissPenalty(int penalty_cycles)
{
    /*	code for setting up miss penaly			    */
    /*	You may add additional code if you need to	*/	
    miss_penalty = penalty_cycles;
}

/* Please declare and implement additional functions for your cache */
