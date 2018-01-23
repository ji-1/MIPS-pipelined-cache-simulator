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
typedef struct Cache_Block_Info {
    int valid;
    uint32_t tag;
    int dirty;
    char LRU;
} Block_Info;


typedef struct Cache_Info {
    Block_Info* block;
} Cache_Set_Info;

int miss_penalty; // number of cycles to stall when a cache miss occurs
uint32_t ***Cache; // data cache storing data [set][way][byte]
Cache_Set_Info *Cache_Info;
int n_set;
int n_way;
int n_byte;
void setupCache(int, int, int);
void setCacheMissPenalty(int);
uint32_t cache_read_32(uint32_t);
void cache_write_32(uint32_t, uint32_t);

