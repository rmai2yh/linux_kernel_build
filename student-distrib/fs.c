/* fs.c -- read-only 391 filesystem driver
 * vim:ts=4 noexpandtab
 */

#include "fs.h"
#include "lib.h" //strcmp

/* 
 * init_fs
 * Initializes the publicly accessible filesystem variables.
 * boot_block: the boot block with length information at the base address
 * dentries: the array of up to 63 directory entries right after the boot block
 * inodes: an array of index nodes starting 1 4kb block after the boot block 
 * data_blocks: an array of index nodes starting 1 + num_inodes blocks after the boot block
 *
 * Inputs: fs_base_address -- the base address of the filesystem, provided by multiboot
 * Returns: None
 * Side effects: initializes the above structures
 */
void init_fs(uint32_t fs_base_address) {
	// Read the function description to understand why the specific values are used.
	boot_block = (boot_block_t*)fs_base_address;
	dentries = (dentry_t*)(fs_base_address + FS_METADATA_SEGMENT_SIZE);
	inodes = (inode_block_t*)(fs_base_address + FS_BLOCK_SIZE);
	data_blocks = (data_block_t*)(fs_base_address + ((boot_block->num_inodes + 1) * FS_BLOCK_SIZE)); 
}

/* read_dentry_by_index
 *
 * Fills a dentry with the values of the dentry in the filesystem at the
 * provided index.
 *
 * Inputs: index -- the index of the dentry in the FS to read
 *         dentry -- the preallocated dentry pointer to update with new values
 * Returns: 0 for success, -1 for failure
 * Side effects: passed-in dentry values overwritten
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
	/* validity checks: is the index within bounds? */
	if (index < 0 || index >= MAX_NUM_DENTRIES)
		return -1;
	/* since file_name is a string, we have to call strncpy to copy all 32 bytes over */
	strncpy(dentry->file_name, dentries[index].file_name, 32);
	/* update the file_type and inode_index ints */
	dentry->file_type = dentries[index].file_type;
	dentry->inode_index = dentries[index].inode_index;
	/* return success */
	return 0;
}

/* read_dentry_by_name
 *
 * Iterates through all the directory entries and searches for one with
 * the name passed in as fname.  If a match is found, update the preallocated
 * passed-in dentry with the values of the match.
 *
 * Inputs: fname -- file name to find
 *         dentry -- the directory entry whose values to update if a match is found
 * Returns: 0 for success or -1 if no match is found
 * Side effects: passed-in dentry values overwritten
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
	int i; // iterator
	if(!strlen((int8_t*)fname)) //if empty string
		return -1;
	for (i = 0; i < MAX_NUM_DENTRIES; i++) { // iterate through all known dentries
		if (strncmp((const int8_t*)fname, dentries[i].file_name, 32) == 0) {
			// found a match!
			// use strncpy to copy the file name
			strncpy(dentry->file_name, dentries[i].file_name, 32);
			// update the passed-in dentry with the ints in the match
			dentry->file_type = dentries[i].file_type;
			dentry->inode_index = dentries[i].inode_index;
			// return success
			return 0;
		}
	}
	// no match found
	return -1;
}

/* read_data
 *
 * Reads data from a provided inode index.  The read should start at offset, and fill
 * the passed in buffer with length number of bytes starting at that point
 *
 * Inputs: inode -- the index of the inode to read data from
 *         offset --  the index of the first byte in the read
 *         buf -- the externally allocated buffer that the read data fills
 *         length -- the number of bytes to fill the buffer with before stopping
 * Returns: # bytes read on success, -1 on failure
 * Side Effects: Overwrites length bytes of the passed in buffer
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
	/* validity checks: are we going to try to read more bytes than are in the file? */
	if ((inode < 0) || (inode >= boot_block->num_inodes) || (offset + length > inodes[inode].length))
		return -1;
	// check all inode data blocks valid index
	int i;
	for(i = 0; i <= inodes[inode].length/FS_BLOCK_SIZE; i++){
		if((inodes[inode].data_index[i] < 0) || (inodes[inode].data_index[i] >= boot_block->num_data_blocks))
			return -1;
	}
	// Offset may indicate a data block other than the first one
	// What block should we start reading from?
	uint32_t inode_data_block_index = offset / FS_BLOCK_SIZE;
	// Keep track of the index of the current data block to speed up lookups
	// the compiler may have optimized this anyways but I don't trust technology.
	uint32_t curr_data_block = inodes[inode].data_index[inode_data_block_index];
	// The offset of the current block should be in range 0 to FS_BLOCK_SIZE-1
	offset %= FS_BLOCK_SIZE;
	// keep track of the number of bytes written
	int bytes_written = 0;
	// iterate through all the bytes we need to write
	while (bytes_written < length) {
		// if we reach the end of a block, we will need to loop around to the next one.
		if (offset >= FS_BLOCK_SIZE) {
			inode_data_block_index++;
			// same values as above, just a new inode block index
			curr_data_block = inodes[inode].data_index[inode_data_block_index];
			offset = 0;
		}
		// copy the byte over and iterate the bytes written (output to buffer)
		// and offset (index for source data)
		buf[bytes_written] = data_blocks[curr_data_block].data[offset];
		bytes_written++;
		offset++;
	}
	// return success
	return bytes_written;
}


/* file_open
 * Description: opens file
 * Inputs: filename
 * Outputs: 0
 * Side Effects: None
 */

int32_t file_open(const uint8_t * filename){
	return 0;
}


/* file_close
 * Description: closes file
 * Inputs: filename
 * Outputs: 0
 * Side Effects: None
 */

int32_t file_close(int32_t fd){
    return 0;
}

/* file_write
 * Description: Writes to the file
 * Inputs:  fd : none
            buf: buffer to write to screen
            nbytes: number of bytes
 * Outputs: -1, since the file system is read only
 * Side Effects: None
 */

int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
    
    //Write calls to regular files should always return -1 since the file system is read only
    return -1;
}

/*
 * file_read
 *   DESCRIPTION:	Reads a given file after extracting the pcb information.
 *					After obtaining the inode index, we then read the data at a
 *					specific file position into the buf and incrememnt the file
 * 					position.
 *   INPUTS: 		fd : fild descriptor
 * 					buf: buffer to copy data to
 * 					nbytes: number of bytes
 *   OUTPUTS:		Number of bytes read
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Populates the buf with file information.
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
	uint32_t inode_idx;
	int bytes_read;
	int position;

	pcb_t * curr_pcb = get_current_executing_pcb();
	position = curr_pcb->fd_array[fd].file_position;
	inode_idx = curr_pcb->fd_array[fd].inode_index;

	if((inodes[inode_idx].length - position) < nbytes)
		nbytes = inodes[inode_idx].length - position;

	bytes_read = read_data(inode_idx, position, buf, nbytes);
	curr_pcb->fd_array[fd].file_position += bytes_read;
	return bytes_read;
}

/*
 * dir_read
 *   DESCRIPTION:	Reads a given directory at a time after extracting pcb
 * 					information. Once finding a populated dentry, we extract
 *					file name, null terminate the name, and then copy it to the
 *					buffer passed in.
 *   INPUTS: 		fd : fild descriptor
 * 					buf: buffer to copy data to
 * 					nbytes: number of bytes
 *   OUTPUTS:		Number of bytes in filename or max of 32
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Populates the buf with a null terminated file name.
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {
	dentry_t dentry;
	int i;

	// find the next populated directory entry
	pcb_t * curr_pcb = get_current_executing_pcb();
	i = curr_pcb->fd_array[fd].file_position; // starting dentry
	while (i < MAX_NUM_DENTRIES) {
		if (read_dentry_by_index(i, &dentry) != 0)
			return 0;
		i++;
		if (dentry.file_name[0] != '\0') { // we found a populated dentry
			strncpy((int8_t*)buf, (int8_t*)dentry.file_name, nbytes);
			break;
		}
	}
	curr_pcb->fd_array[fd].file_position = i;
	return strlen(dentry.file_name) > nbytes ? nbytes : strlen(dentry.file_name);
}

