#include "L1Cache.h"
#include "Cache.h"

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache SimpleCache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

    if (address >= DRAM_SIZE - WORD_SIZE + 1)
        exit(-1);

    if (mode == MODE_READ) {
        memcpy(data, &(DRAM[address]), BLOCK_SIZE);
        time += DRAM_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(&(DRAM[address]), data, BLOCK_SIZE);
        time += DRAM_WRITE_TIME;
    }
}

/*********************** L1 cache *************************/

void initCache() { SimpleCache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

    uint32_t index, tag, offset, word_offset, mem_address, temp_address,
        cache_address, cache_block_address;
    uint8_t temp_block[BLOCK_SIZE];

    /* init cache */
    if (SimpleCache.init == 0) {
        for (int i = 0; i < L1_BLOCK_COUNT; i++) {
            SimpleCache.line[i].Valid = 0;
        }
        SimpleCache.init = 1;
    }

    /* get offset (word + byte), word offset, index and tag */
    offset = address & 0x0000003F;
    word_offset = offset >> 2;
    index = (address >> 6) & 0x000000FF;
    tag = address >> 14;

    CacheLine *Line = &SimpleCache.line[index]; // Cache Line structure

    /* get block address in DRAM */
    mem_address = address >> 6;
    mem_address = mem_address << 6;

    /* block and word address inside the cache */
    cache_block_address = (index * BLOCK_SIZE);
    cache_address = (index * BLOCK_SIZE) + (word_offset * WORD_SIZE);

    /* access Cache */
    if (!Line->Valid || Line->Tag != tag) { // if block not present - miss
        accessDRAM(mem_address, temp_block,
                   MODE_READ); // get new block from DRAM

        if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
            /* temp_address is DRAM address for the dirty tag */
            temp_address = index << 6;
            temp_address |= Line->Tag << 14;
            accessDRAM(temp_address, &(L1Cache[0]) + cache_block_address,
                       MODE_WRITE);
        }

        memcpy(&(L1Cache[0]) + (index * BLOCK_SIZE), temp_block, BLOCK_SIZE);
        Line->Valid = 1;
        Line->Tag = tag;
        Line->Dirty = 0;
    }

    if (mode == MODE_READ) { // read data from cache line
        memcpy(data, &(L1Cache[0]) + cache_address, WORD_SIZE);
        time += L1_READ_TIME;
    } else if (mode == MODE_WRITE) { // write data from cache line
        memcpy(&(L1Cache[0]) + cache_address, data, WORD_SIZE);
        time += L1_WRITE_TIME;
        Line->Dirty = 1;
    }
}

void read(uint32_t address, uint8_t *data) {
    accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
    accessL1(address, data, MODE_WRITE);
}
