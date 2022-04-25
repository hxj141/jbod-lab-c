//CMPSC 311 SP22
//LAB 5

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

//Defining parameters for easier use later
#define DISK_COUNT 16
#define BLOCK_LEN 256
#define BLOCK_COUNT 256
#define DISK_SIZE (BLOCK_LEN*BLOCK_COUNT)
#define TOTAL_CAPACITY (DISK_COUNT*DISK_SIZE)


uint32_t jbod(uint32_t command, uint32_t disk, uint32_t reserved, uint32_t block) { //Construct the operation bytestring

	uint32_t Blockid = block; //blockid parameter
	uint32_t Command = 0; //command parameter 
	uint32_t Diskid = 0; //diskid parameter
	uint32_t Reserved = 0; //reserved space
	

	Command =  (uint32_t) command << 26;
	Diskid =  (uint32_t) disk << 22;
	Reserved = (uint32_t) reserved << 8; // get each of the parameters into their respective positions, leaving the empty space of the bytestrings equal to zero


	uint32_t op = Blockid | Reserved | Diskid | Command; //By using a chain of or statements, this will combine all the parameters into one bytestring containing all the information in the right place 
	return op;
}

int mdadm_mount(void){
int status = jbod_client_operation(jbod(JBOD_MOUNT,0,0,0), NULL); //as per the jbod_operation parameters, having the opcode and then a null block is how you mount
  if (status == 0) { //if the jbod process returns 0, we know that it succeeded, so we return a success signal of 1
  	return 1; 
  }
  return -1;
}
int mdadm_unmount(void) {
int status = jbod_operation(jbod(JBOD_UNMOUNT,0,0,0), NULL); //unmount takes an identical structure to mount
  if (status == 0) {
  	return 1;
  }
  return -1;
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
	// Handling invalid input parameters
	if (len > 1024) { //Fails when length is out of bounds
		return -1;
	}

	if ((len != 0) && (buf == NULL)) { //Fails when passed a null pointer w/ non-zero length
		return -1;
	}

	if (addr + len > TOTAL_CAPACITY) { //Tests for out of bounds address accesses
		return -1;
	}

	//Seeking to relevant place in memory
	int diskid = addr/65536; //65536 bytes per disk, so dividing addr by 65536 will yield disk
	int blockid = (addr%65536)/256; //Once the disk has been selected, read bytes 

	int seekdisk = jbod_client_operation(jbod(JBOD_SEEK_TO_DISK,diskid,0,0), NULL); //seeks to disk
	int seekblock = jbod_client_operation(jbod(JBOD_SEEK_TO_BLOCK,0,0,blockid), NULL); //locates the block next
		
	if (seekdisk == -1) // If seekdisk fails, return failure on function
	{
		return -1;
	}
	if (seekblock == -1) // If seekdisk fails, return failure on function
	{
		return -1;
	}
	//Check to see if this can be gotten in cache; first check if enabled, then try to look up and read
	if (cache_enabled() == true) {
		cache_lookup(diskid, blockid, buf);
		jbod_client_operation(jbod(JBOD_READ_BLOCK,0,0,0), buf);
	}
	//Preparing for and performing the actual READ operation
	uint8_t temp[256]; //Buffer to represent the currently read section of memory
	uint32_t i = 0; //Set the incrementer

	// Read across blocks. Plan is to the chunk on the first block, copy that into the buffer, align, then copy the chunk in the second block into the buffer. Joining them together will provide the whole section of memory.
	// Copying first chunk into buffer. Also used when reading across disks. 
	uint32_t remaining_len = addr % BLOCK_LEN; //Amount of bytes to copy. Defined as distance from boundary to the first block. Additional modulo to ensure multiples of 256 are functionally zero.
	if (remaining_len != 0) { //Formula in the above line will equal zero if buffer is contained within this section of memory. Checking for it will tell us if this is across blocks.
		jbod_client_operation(jbod(JBOD_READ_BLOCK,0,0,0), temp); //Reading data into temp
		if(len < 256 - remaining_len) {
			memcpy(buf, &temp[remaining_len], len); // Copies the data in the first block over, the second block's data will get copied over in line 111
			i += len; //Setting where in buffer the next block's data is going to go to properly combine
		}
		else {
			memcpy(buf, &temp[remaining_len], 256 - remaining_len); // Copies the data in the first block over, the second block's data will get copied over in line 111
			i += 256 - remaining_len; //Setting where in buffer the next block's data is going to go to properly combine
		}
	}


	int copylen = BLOCK_LEN;

	//Handling the second chunk
	for(;i < len; i+= copylen) { //For each block we want to read

		diskid = (addr+i)/65536; //Get diskid similar to how it was gotten before
		blockid = ((addr+i)%65536)/256; //Get blockid similar to how it was gotten before
		seekdisk = jbod_client_operation(jbod(JBOD_SEEK_TO_DISK, diskid, 0, 0), NULL); //Seek to the new disk
		seekblock = jbod_client_operation(jbod(JBOD_SEEK_TO_BLOCK, 0, 0, blockid), NULL); //Seek to the 0th block of the new disk

		if(seekdisk == -1) {
			return -1;
		}
		if(seekblock == -1) {
			return -1;
		}

		copylen = BLOCK_LEN; //Default copylen to the standard block length

		if (len - i < BLOCK_LEN) { //If we need to copy less than a full block, then we adjust the copylen size so it won't go out of bounds
			copylen = len - i;
		}

		jbod_client_operation(jbod(JBOD_READ_BLOCK,0,0,0), temp); //Reading that section of the disk
  		memcpy(&buf[i],temp,copylen); //Copying that section into the buffer; if across blocks, second block's data is joined to the first blocks, giving the full data combined in the buffer
	}
	return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
	// Handling invalid input parameters
    if (len > 1024) { //Fails when length is out of bounds
    	return -1;
    }
  
    if ((len != 0) && (buf == NULL)) { //Fails when passed a null pointer w/ non-zero length
    	return -1;
    }
  
    if (addr + len > TOTAL_CAPACITY) { //Tests for out of bounds address accesses
    	return -1;
    }
  
  
  	//Seeking to relevant place in memory
    int diskid = addr/65536; //65536 bytes per disk, so dividing addr by 65536 will yield disk
    int blockid = (addr%65536)/256; //Once the disk has been selected, read bytes 
  
    int seekdisk = jbod_client_operation(jbod(JBOD_SEEK_TO_DISK,diskid,0,0), NULL); //seeks to disk
    int seekblock = jbod_client_operation(jbod(JBOD_SEEK_TO_BLOCK,0,0,blockid), NULL); //locates the block next
        
    if (seekdisk == -1) // If seekdisk fails, return failure on function
    {
    	return -1;
    }
    if (seekblock == -1) // If seekdisk fails, return failure on function
    {
    	return -1;
    }

	//Preparing for and performing the actual READ operation
    uint8_t temp[256]; //Buffer to represent the currently read section of memory
    uint32_t i = 0; //Set the incrementer

	// Read across blocks. Plan is to the chunk on the first block, copy that into the buffer, align, then copy the chunk in the second block into the buffer. Joining them together will provide the whole section of memory.
    // Copying first chunk into buffer. Also used when reading across disks. 
	uint32_t remaining_len = addr % BLOCK_LEN; //Amount of bytes to copy. Defined as distance from boundary to the first block. Additional modulo to ensure multiples of 256 are functionally zero.
	if (remaining_len != 0) { //Formula in the above line will equal zero if buffer is contained within this section of memory. Checking for it will tell us if this is across blocks.
		jbod_client_operation(jbod(JBOD_READ_BLOCK,0,0,0),temp); //Read data currently at space into temp
		//Put pointer back to where it was before read
		jbod_client_operation(jbod(JBOD_SEEK_TO_DISK,diskid,0,0), NULL);
		jbod_client_operation(jbod(JBOD_SEEK_TO_BLOCK,0,0,blockid),NULL); 
		if(len < 256 - remaining_len) {
			memcpy(&temp[remaining_len], buf, len); // Copies the data in the first block over, the second block's data will get copied over in line 111
			jbod_client_operation(jbod(JBOD_WRITE_BLOCK,0,0,0), temp); //Writing data from temp
			i += len; //Setting where in buffer the next block's data is going to go to properly combine
		}
		else {
			memcpy(&temp[remaining_len], buf, 256 - remaining_len); // Copies the data in the first block over, the second block's data will get copied over in line 111
			jbod_client_operation(jbod(JBOD_WRITE_BLOCK,0,0,0), temp); //Writing data from temp
			i += 256 - remaining_len; //Setting where in buffer the next block's data is going to go to properly combine
		}
    }
	int copylen = BLOCK_LEN;

	//Handling the second chunk
	for(;i < len; i+= copylen) { //For each block we want to read

		diskid = (addr+i)/65536; //Get diskid similar to how it was gotten before
		blockid = ((addr+i)%65536)/256; //Get blockid similar to how it was gotten before
		seekdisk = jbod_client_operation(jbod(JBOD_SEEK_TO_DISK, diskid, 0, 0), NULL); //Seek to the new disk
		seekblock = jbod_client_operation(jbod(JBOD_SEEK_TO_BLOCK, 0, 0, blockid), NULL); //Seek to the 0th block of the new disk

		if(seekdisk == -1) {
			return -1;
		}
		if(seekblock == -1) {
			return -1;
		}

		copylen = BLOCK_LEN; //Default copylen to the standard block length

		if (len - i < BLOCK_LEN) { //If we need to copy less than a full block, then we adjust the copylen size so it won't go out of bounds
			copylen = len - i;
		}

		jbod_client_operation(jbod(JBOD_READ_BLOCK,0,0,0), temp); //Reading that section of the disk
		jbod_client_operation(jbod(JBOD_SEEK_TO_DISK, diskid, 0, 0), NULL); //Seek to the new disk
		jbod_client_operation(jbod(JBOD_SEEK_TO_BLOCK, 0, 0, blockid), NULL); //Seek to the 0th block of the new disk

  		memcpy(temp,&buf[i],copylen); //Copying that section into the buffer; if across blocks, second block's data is joined to the first blocks, giving the full data combined in the buffer
		jbod_client_operation(jbod(JBOD_WRITE_BLOCK,0,0,0), temp);  //Reading that section of the disk
	}
	return len;
}
