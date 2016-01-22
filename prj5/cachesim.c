#include "cachesim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef struct {
    uint64_t myTag;
    int valid;
    int dirty;
    int lru;
} cacheEntry_;
typedef struct {
    cacheEntry_ * line;
}set_;
uint64_t block_bit;
uint64_t index_bit;
uint64_t tag_bit;
uint64_t numBlock;
uint64_t num_lines;
uint64_t setSize;
uint64_t num_set;
set_* myCache;
int p = 0;
int hit = 0;
int invalid = 0;
int miss = 0;
/**
 * Subroutint for initializing your cache with the paramters.
 * You may add and initialize any global variables that you might need
 *
 * @param C The total size of your cache is 2^size bytes
 * @param S The total number of sets in your cache are 2^S
 * @param B The size of your block is 2^B bytes
 */
void cache_init(uint64_t C, uint64_t S, uint64_t B) {
    block_bit = B;
    setSize = C - S; //the size of per set
    numBlock = 1 << (C - B); // the number of total block
    index_bit = C - B - S; // the number of block per set
    num_lines = 1 << index_bit;
    tag_bit = 64 - index_bit - block_bit;
    num_set = 1 << S;
    //number of cache * size of set
    myCache = (set_*)calloc(num_set, sizeof(set_));
    int i;
    int j;
    //initialize every set
    for (i = 0; i < num_set; i++) {
        myCache[i].line = (cacheEntry_*)calloc(num_lines, sizeof(cacheEntry_));
    }
    //initialize all blocks
    for (i = 0; i < num_set; i++) {
        for (j = 0; j < num_lines; j++) {
            myCache[i].line[j].valid = 0;
            myCache[i].line[j].dirty = 0;
            myCache[i].line[j].myTag = 0;
            myCache[i].line[j].lru = -1;
        }
    }
}

/**
 * Subroutine that simulates one cache event at a time.
 * @param rw The type of access, READ or WRITE
 * @param address The address that is being accessed
 * @param stats The struct that you are supposed to store the stats in
 */
void cache_access (char rw, uint64_t address, struct cache_stats_t *stats) {
    uint64_t tag = address >> (index_bit + block_bit);
    uint64_t index = (address >> (block_bit)) & (num_lines - (uint64_t)1);
    // printf("C: %" PRIu64 "\n", address);
    // printf("B: %" PRIu64 "\n", tag);
    // printf("S: %" PRIu64 "\n", index);
//    uint64_t offset = address % block_bit;
    stats->accesses = (stats->accesses) + 1;
    int i;
    int j;
    //if it is write
    if (rw == 'r') {
        stats->reads = (stats->reads) + 1;
        //if there is a hit
        for (i = 0; i < num_set; i++) {
            if (myCache[i].line[index].myTag == tag && myCache[i].line[index].valid) {
                // printf("hit %d", p);
                // p++;
                myCache[i].line[index].valid = 1;
                for (j = 0; j < num_set; j++) {
                    if (myCache[j].line[index].lru != -1) {
                        myCache[j].line[index].lru++;
                    }
                }
                myCache[i].line[index].lru = 0;
                hit++;
                return;
            }
        }
        //if there is a miss and find one that is not valid
        for (i = 0; i < num_set; i++) {
            if (!myCache[i].line[index].valid) {
                // printf("look me %d", p);
                // p++;
                invalid++;
                myCache[i].line[index].myTag = tag;
                myCache[i].line[index].valid = 1;
                myCache[i].line[index].dirty = 0;
                for (j = 0; j < num_set; j++) {
                    if (myCache[j].line[index].lru != -1) {
                        myCache[j].line[index].lru++;
                    }
                }
                myCache[i].line[index].lru = 0;
                stats->read_misses = stats->read_misses + 1;
                return;
            }
        }
        //if there is a miss that does not find a invalid block
        //replace the block
        int max = myCache[0].line[index].lru;
        int maxTag = myCache[0].line[index].myTag;
        int k;
        int q = 0;
        for (k = 1; k < num_set; k++) {
            if (myCache[k].line[index].lru > max) {
                max = myCache[k].line[index].lru;
                maxTag = myCache[k].line[index].myTag;
                q = k;
            }
        }
        // for (i = 0; i < num_set; i++) {
            // if (myCache[i].line[index].myTag == maxTag) {
                if (myCache[q].line[index].dirty == 1) {
                    stats->write_backs = stats->write_backs + 1;
                }
                // printf("look me");
                miss++;
                myCache[q].line[index].myTag = tag;
                myCache[q].line[index].dirty = 0;
                myCache[q].line[index].valid = 1;
                for (j = 0; j < num_set; j++) {
                    if (myCache[j].line[index].lru != -1) {
                        myCache[j].line[index].lru++;
                    }
                }
                myCache[q].line[index].lru = 0;
                stats->read_misses = stats->read_misses + 1;
                return;
            // }
        // }



    } else {
        stats->writes = (stats->writes) + 1;
        //if there is hit
        for (i = 0; i < num_set; i++) {
            if (myCache[i].line[index].myTag == tag) {
                myCache[i].line[index].valid = 1;
                myCache[i].line[index].dirty = 1;
                //increment access time for that block
                for (j = 0; j < num_set; j++) {
                    if (myCache[j].line[index].lru != -1) {
                        myCache[j].line[index].lru++;
                    }
                }
                //set this block's access time to be 0
                myCache[i].line[index].lru = 0;
                return;
            }
        }
        //if there is a miss and find one that is not valid
        for (i = 0; i < num_set; i++) {
            if (!myCache[i].line[index].valid) {
                myCache[i].line[index].myTag = tag;
                myCache[i].line[index].valid = 1;
                myCache[i].line[index].dirty = 1;
                for (j = 0; j < num_set; j++) {
                    if (myCache[j].line[index].lru != -1) {
                        myCache[j].line[index].lru++;
                    }
                }
                invalid++;
                myCache[i].line[index].lru = 0;
                stats->write_misses = stats->write_misses + 1;
                return;
            }
        }
        //if there is a miss that does not find a invalid block
        //replace the block
        int max = myCache[0].line[index].lru;
        int maxTag = myCache[0].line[index].myTag;
        int k;
        int q = 0;
        for (k = 1; k < num_set; k++) {
            if (myCache[k].line[index].lru > max) {
                max = myCache[k].line[index].lru;
                maxTag = myCache[k].line[index].myTag;
                q = k;
            }
        }
        // for (i = 0; i < num_set; i++) {
        //     if (myCache[i].line[index].myTag == maxTag) {
                if (myCache[q].line[index].dirty == 1) {
                    stats->write_backs = stats->write_backs + 1;
                }
                myCache[q].line[index].myTag = tag;
                myCache[q].line[index].dirty = 1;
                myCache[q].line[index].valid = 1;
                for (j = 0; j < num_set; j++) {
                    if (myCache[j].line[index].lru != -1) {
                        myCache[j].line[index].lru++;
                    }
                }
                myCache[q].line[index].lru = 0;
                stats->write_misses = stats->write_misses + 1;
                return;
        //     }
        // }
    }
}


/**
 * Subroutine for cleaning up memory operations and doing any calculations
 *
 */
void cache_cleanup (struct cache_stats_t *stats) {
    int i;
    int j;
    //clean up all blocks
    for (i = 0; i < num_set; i++) {
        for (j = 0; j < num_lines; j++) {
            myCache[i].line[j].valid = 0;
            myCache[i].line[j].dirty = 0;
            myCache[i].line[j].myTag = 0;
            myCache[i].line[j].lru = 0;
        }
    }
    free(myCache->line);
    free(myCache);
    // printf("hit: %d\n", hit);
    // printf("invalid%d\n", invalid);
    // printf("miss :%d\n", miss);
    // printf("setSize: %llu\n", setSize);
    // printf("numBlock: %llu\n", numBlock);
    // printf("num_lines: %llu\n", num_lines);
    // printf("index_bit: %llu\n", index_bit);
    // printf("tag_bit: %llu\n", tag_bit);
    // printf("num_set: %llu\n", num_set);
    // printf("block_bit: %llu\n",block_bit);
    stats->misses = stats->write_misses + stats->read_misses;
    stats->miss_rate = 1.0 * stats->misses / stats->accesses;
    stats->avg_access_time = stats->access_time + stats->miss_rate * stats->miss_penalty;

}