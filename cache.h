/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   cache.h                                                   */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* cache.h : Declare functions and data necessary for your project*/
struct Data {
    byte element[8];
};

struct CacheBlock {
    bool valid;
    uint32_t tag;
    Data data;
};

int miss_penalty; // number of cycles to stall when a cache miss occurs
uint32_t ***Cache; // data cache storing data [set][way][byte]

void setupCache(int, int, int);
void setCacheMissPenalty(int);
