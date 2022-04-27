#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

#define DISK_COUNT 16
#define BLOCK_LEN 256
#define BLOCK_COUNT 256
#define DISK_SIZE (BLOCK_LEN*BLOCK_COUNT)
#define TOTAL_CAPACITY (DISK_COUNT*DISK_SIZE)



static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;



int cache_create(int num_entries) {
  //last 6 are cache alone
  //initialize cache to the maximum
  //Checking to see if entry size is valid
  int valid_size = 0;
  if ((num_entries > 1) & (num_entries < 4097)) {
  	valid_size = 1;
  }

  if ((cache == NULL) & (valid_size == 1)) {
  	cache_size = num_entries; //Set the cache size equal to the number of entries
  	cache_entry_t *cache_loc = malloc(sizeof(cache_entry_t)*num_entries); //Allocate the memory
  	
  	if (cache_loc == NULL) { //If the allocation fails, return -1
  		return -1;
  	}  	
  	else {
		cache = cache_loc; //Store address in cache
  		return 1; 
	}
  }  
  else { //If function is called twice in a row, fail
  	return -1;
  }
  
}

int cache_destroy(void) {
  if (cache != NULL) { //Avoids calling the function twice in a row
  	free(cache);
	cache = NULL;
	cache_size = 0;
	return 1;
  }
  else { //If function is called twice in a row, fail
  	return -1;
  }
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  //Accounting for invalid parameters
  if ((disk_num > (DISK_COUNT - 1)) | (block_num > (BLOCK_COUNT - 1))) { 
		return -1;
	}
  if ((disk_num < 0) | (block_num < 0)) {
		return -1;
	}
  if ((cache != NULL) & (cache_size != 0)) {
	  if (cache[0].valid == true) { 
		for (int i = 0; i < cache_size; i++) { //Check each entry
			num_queries += 1; // Increment queries by 1
			if ((cache[i].disk_num == disk_num) & (cache[i].block_num == block_num)) { //If the disk number and the block number match the query
				memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE); //Copy entry to buffer
				num_hits += 1; //Increment number of hits
				clock += 1; //Increment clock
				cache[i].access_time = clock; //Update access time for the entry
				return 1;
			}
		}  
    	}
	}
  	return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
	for (int i = 0; i < cache_size; i++) { //Check each entry
		if ((cache[i].disk_num == disk_num) & (cache[i].block_num == block_num)) { //If the disk number and the block number match the query
			memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
			clock += 1; //Increment clock
			cache[i].access_time = clock; //Update access time for the entry
		}
	}
}
int cache_insert(int disk_num, int block_num, const uint8_t *buf) {

  //Accounting for invalid parameters
	    //If either cache or buffer doesn't exist
	  	if ((buf == NULL) | (cache == NULL)) { 
		  	return -1;
		  }
		// If inserting a block which already exists in cache
		for (int i = 0; i < cache_size; i++) {
			if ((cache[i].disk_num == disk_num) & (cache[i].block_num == block_num) & (cache[i].valid == true)) {
				return -1;
			}	
		}
		//If disk or block parameters are out of range
		if ((disk_num > (DISK_COUNT - 1)) | (block_num > (BLOCK_COUNT - 1))) { //If too high index
			return -1;
		}
		if ((disk_num < 0) | (block_num < 0)) { //If too low index
			return -1;
		}
		
		else {
	if (cache[cache_size - 1].valid == true) { //If the last entry of the cache exists, then the cache is full and LRU should be triggered
				int current_lru = 0;
				int current_lru_time = cache[0].access_time;
				for (int i = 0; i < cache_size; i++) { //Scan cache, updating the LRU if a value is lower; lower the clock, the less recently used it is
					if (cache[i].access_time < current_lru_time) {
						current_lru_time = cache[i].access_time; //Keep updating the lowest time
						current_lru = i;
					}
				// Whatever current_lru happens to be at the end, replace its values with the new ones
				clock += 1; //Increment clock
				cache[current_lru].disk_num = disk_num;
				cache[current_lru].block_num = block_num;				
				cache[current_lru].valid = true; 
				cache[current_lru].access_time = clock;
				memcpy(cache[current_lru].block, buf, JBOD_BLOCK_SIZE);
				return 1; 	
				}
			}
	else {
			//Otherwise insert new entry into
			for (int i = 0; i < cache_size; i++) { //Keep looping through cache until you find something empty
				if (cache[i].valid == false) { //If you find something empty, write to there
					//Setting parameters
					clock += 1; //Increment clock
					cache[i].disk_num = disk_num;
					cache[i].block_num = block_num;
					cache[i].valid = true; 
					cache[i].access_time = clock;
					memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
					return 1; 
				}
			
			}
		}		
	}	
  return -10;

}


bool cache_enabled(void) {
  if (cache_size >= 2) { //Enabled is being defined as per the instructions as cache size being larger than 2
  	return true;
  } 
  else {
  	return false;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}


//check if cache enabled, then lookup, then 