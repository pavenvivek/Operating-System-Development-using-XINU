#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>

static fsystem_t fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0 // Superblock
#define BM_BLK 1 // Bitmapblock

#define NUM_FD 16

filetable_t oft[NUM_FD]; // open file table
#define isbadfd(fd) (fd < 0 || fd >= NUM_FD || oft[fd].in.id == EMPTY)

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(inode_t))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2
#define FIRST_DATA_BLOCK 15

/**
 * Helper functions
 */
int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEDIRECTBLOCKS) {
    errormsg("No indirect block support! (%d >= %d)\n", fileblock, INODEBLOCKS - 2);
    return SYSERR;
  }

  // Get the logical block address
  diskblock = oft[fd].in.blocks[fileblock];

  return diskblock;
}

/**
 * Filesystem functions
 */
int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out) {
  int bl, inn;
  int inode_off;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %s)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl  = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(inode_t);

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(out, &block_cache[inode_off], sizeof(inode_t));

  return OK;

}

int _fs_put_inode_by_num(int dev, int inode_number, inode_t *in) {
  int bl, inn;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %d)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(inode_t))], in, sizeof(inode_t));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes) {
  int i;

  if (dev == dev0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  } else {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  } else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) { i++; }
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *) SYSERR) {
    errormsg("fs_mkfs memget failed\n");
    return SYSERR;
  }

  /* zero the free mask */
  for(i = 0; i < fsd.freemaskbytes; i++) {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(fsystem_t));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  // Initialize all inode IDs to EMPTY
  inode_t tmp_in;
  for (i = 0; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    tmp_in.id = EMPTY;
    _fs_put_inode_by_num(dev0, i, &tmp_in);
  }
  fsd.root_dir.numentries = 0;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
  }

  for (i = 0; i < NUM_FD; i++) {
    oft[i].state     = 0;
    oft[i].fileptr   = 0;
    oft[i].de        = NULL;
    oft[i].in.id     = EMPTY;
    oft[i].in.type   = 0;
    oft[i].in.nlink  = 0;
    oft[i].in.device = 0;
    oft[i].in.size   = 0;
    memset(oft[i].in.blocks, 0, INODEBLOCKS);
    oft[i].flag      = 0;
  }

  return OK;
}

int fs_freefs(int dev) {
  if (freemem(fsd.freemask, fsd.freemaskbytes) == SYSERR) {
    return SYSERR;
  }

  return OK;
}

/**
 * Debugging functions
 */
void fs_print_oft(void) {
  int i;

  printf ("\n\033[35moft[]\033[39m\n");
  printf ("%3s  %5s  %7s  %8s  %6s  %5s  %4s  %s\n", "Num", "state", "fileptr", "de", "de.num", "in.id", "flag", "de.name");
  for (i = 0; i < NUM_FD; i++) {
    if (oft[i].de != NULL) printf ("%3d  %5d  %7d  %8d  %6d  %5d  %4d  %s\n", i, oft[i].state, oft[i].fileptr, oft[i].de, oft[i].de->inode_num, oft[i].in.id, oft[i].flag, oft[i].de->name);
  }

  printf ("\n\033[35mfsd.root_dir.entry[] (numentries: %d)\033[39m\n", fsd.root_dir.numentries);
  printf ("%3s  %3s  %s\n", "ID", "id", "filename");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (fsd.root_dir.entry[i].inode_num != EMPTY) printf("%3d  %3d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
  printf("\n");
}

void fs_print_inode(int fd) {
  int i;

  printf("\n\033[35mInode FS=%d\033[39m\n", fd);
  printf("Name:    %s\n", oft[fd].de->name);
  printf("State:   %d\n", oft[fd].state);
  printf("Flag:    %d\n", oft[fd].flag);
  printf("Fileptr: %d\n", oft[fd].fileptr);
  printf("Type:    %d\n", oft[fd].in.type);
  printf("nlink:   %d\n", oft[fd].in.nlink);
  printf("device:  %d\n", oft[fd].in.device);
  printf("size:    %d\n", oft[fd].in.size);
  printf("blocks: ");
  for (i = 0; i < INODEBLOCKS; i++) {
    printf(" %d", oft[fd].in.blocks[i]);
  }
  printf("\n");
  return;
}

void fs_print_fsd(void) {
  int i;

  printf("\033[35mfsystem_t fsd\033[39m\n");
  printf("fsd.nblocks:       %d\n", fsd.nblocks);
  printf("fsd.blocksz:       %d\n", fsd.blocksz);
  printf("fsd.ninodes:       %d\n", fsd.ninodes);
  printf("fsd.inodes_used:   %d\n", fsd.inodes_used);
  printf("fsd.freemaskbytes  %d\n", fsd.freemaskbytes);
  printf("sizeof(inode_t):   %d\n", sizeof(inode_t));
  printf("INODES_PER_BLOCK:  %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS:  %d\n", NUM_INODE_BLOCKS);

  inode_t tmp_in;
  printf ("\n\033[35mBlocks\033[39m\n");
  printf ("%3s  %3s  %4s  %4s  %3s  %4s\n", "Num", "id", "type", "nlnk", "dev", "size");
  for (i = 0; i < NUM_FD; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) printf("%3d  %3d  %4d  %4d  %3d  %4d\n", i, tmp_in.id, tmp_in.type, tmp_in.nlink, tmp_in.device, tmp_in.size);
  }
  for (i = NUM_FD; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) {
      printf("%3d:", i);
      int j;
      for (j = 0; j < 64; j++) {
        printf(" %3d", *(((char *) &tmp_in) + j));
      }
      printf("\n");
    }
  }
  printf("\n");
}

void fs_print_dir(void) {
  int i;

  printf("%22s  %9s  %s\n", "DirectoryEntry", "inode_num", "name");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    printf("fsd.root_dir.entry[%2d]  %9d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
}

int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
}

int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/**
 * This is maybe a little overcomplicated since the lowest-numbered
 * block is indicated in the high-order bit.  Shift the byte by j
 * positions to make the match in bit7 (the 8th bit) and then shift
 * that value 7 times to the low-order bit to print.  Yes, it could be
 * the other way...
 */
void fs_printfreemask(void) { // print block bitmask
  int i, j;

  for (i = 0; i < fsd.freemaskbytes; i++) {
    for (j = 0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    printf(" ");
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}


int get_next_available_block() {

  int i = 0;
  int maskbit = -1;

  for (i = FIRST_DATA_BLOCK; i < fsd.nblocks; i++) {
	maskbit = fs_getmaskbit(i);

	// check if block is available
	if (maskbit == 0) {
		fs_setmaskbit(i);
		return i;
	}
  }

  return SYSERR;
}


void update_oft_entry (int inode_num) {

  int i= 0;

  for (i = 0; i < NUM_FD; i++) {
  	if (oft[i].in.id == inode_num) {
		_fs_get_inode_by_num(dev0, inode_num, &oft[i].in);
		break;	
	}
  }
}

void delete_oft_entry (char *filename) {

  int i = 0;

  for (i = 0; i < NUM_FD; i++) {
  	if (strlen(oft[i].de->name) == strlen(filename) &&
		memcmp(oft[i].de->name, filename, strlen(filename)) == 0) {
		oft[i].state     = 0;
    		oft[i].fileptr   = 0;
    		oft[i].de        = NULL;
   		oft[i].in.id     = EMPTY;
    		oft[i].in.type   = 0;
    		oft[i].in.nlink  = 0;
    		oft[i].in.device = 0;
    		oft[i].in.size   = 0;
    		memset(oft[i].in.blocks, 0, INODEBLOCKS);
    		oft[i].flag      = 0;
		break;	
	}
  }
}

/**
 * Implementation of filesystem functions:
 */

int fs_open(char *filename, int flags) {

  if (flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR) {
  	return SYSERR;
  }

  int i = 0;

  for (i = 0; i < DIRECTORY_SIZE; i++) {
    	if (fsd.root_dir.entry[i].inode_num != EMPTY && 
		strlen(filename) == strlen(fsd.root_dir.entry[i].name) && 
		memcmp(filename, fsd.root_dir.entry[i].name, strlen(filename)) == 0) {
     		
		if (oft[i].state == FSTATE_OPEN) {
			return SYSERR;  // File is already open
		}

		_fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &oft[i].in);
		
		oft[i].state = FSTATE_OPEN;
		//oft[i].fileptr = 0;
		oft[i].de = &fsd.root_dir.entry[i];
		oft[i].flag = flags;

		return i;
    	}	    
  }


  return SYSERR;
}

int fs_close(int fd) {
 
  if (oft[fd].state == FSTATE_CLOSED) {
  	return SYSERR;  // File is already closed
  }

  oft[fd].state = FSTATE_CLOSED;

  return OK;
}

int fs_create(char *filename, int mode) {

  int i = 0, slot = -1;

  if (mode != O_CREAT) {
  	return SYSERR;
  }
  
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    	if (fsd.root_dir.entry[i].inode_num != EMPTY &&
		strlen(filename) == strlen(fsd.root_dir.entry[i].name) && 
		memcmp(filename, fsd.root_dir.entry[i].name, strlen(filename)) == 0) {
		
		return SYSERR;  // Duplicate file name	
	}
	else if (fsd.root_dir.entry[i].inode_num == EMPTY && slot == -1) {
		slot = i;
	}
  }

  if (slot == -1) {
  	return SYSERR;  // Directory is full
  }

  int next_inode_num = -1;
  inode_t tmp_in;

  for (i = 0; i < fsd.ninodes; i++) {	
        _fs_get_inode_by_num(dev0, i, &tmp_in);

	if (tmp_in.id == EMPTY) {
		next_inode_num = i;
		tmp_in.id = i;
		tmp_in.type = INODE_TYPE_FILE;
		tmp_in.nlink = 1;
		tmp_in.device = dev0;
		tmp_in.size = 0;
    		_fs_put_inode_by_num(dev0, i, &tmp_in);
		fsd.inodes_used = fsd.inodes_used + 1;
		break;
	}
  }

  fsd.root_dir.entry[slot].inode_num = next_inode_num;
  memcpy(fsd.root_dir.entry[slot].name, filename, strlen(filename));
  fsd.root_dir.numentries = fsd.root_dir.numentries + 1;

  int fd = fs_open (filename, O_RDWR);

  return fd;
}

int fs_seek(int fd, int offset) {

  if (oft[fd].de == NULL || oft[fd].state != FSTATE_OPEN) {
  	return SYSERR;
  }
  else if (oft[fd].in.size == 0) {
  	return SYSERR; // File is empty
  } 
  else if (offset < 0 || offset >= oft[fd].in.size) {
  	return SYSERR; // Out of bounds
  }

  oft[fd].fileptr = offset;

  return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
  
  int offset = oft[fd].fileptr;
  int starting_block = offset / fsd.blocksz;
  int block_offset = offset % fsd.blocksz;
  int i = 0, read_length = 0;
  int read_size = 0;

  if (oft[fd].flag == O_WRONLY || oft[fd].state == FSTATE_CLOSED) {
  	return SYSERR;
  }
  
  if (oft[fd].in.size == 0)
	  return 0;
  
  if (nbytes > (oft[fd].in.size - offset)) {
  	nbytes = oft[fd].in.size - offset;
  }


  for (i = starting_block; i < INODEDIRECTBLOCKS; i++) {
	
	if (nbytes - read_length > fsd.blocksz) {
		read_size = fsd.blocksz;
		
		if (i == starting_block) {
			read_size = fsd.blocksz - block_offset;
		}
	}
	else {
		read_size = nbytes - read_length;

		if (i == starting_block) {
			if (fsd.blocksz - block_offset > nbytes) {
				read_size = nbytes;
			}
			else {
				read_size = fsd.blocksz - block_offset;
			}
		}
	}
  	
	bs_bread(oft[fd].in.device, oft[fd].in.blocks[i], block_offset, buf + read_length, read_size);
	read_length += read_size;
	block_offset = 0;
  }
  
  return read_length;
}

int fs_write(int fd, void *buf, int nbytes) {

  int offset = oft[fd].fileptr;
  int starting_block = offset / fsd.blocksz;
  int block_offset = offset % fsd.blocksz;
  int i = 0, write_length = 0;
  int next_available_block;
  int write_size = 0;

  if (oft[fd].flag == O_RDONLY || oft[fd].state == FSTATE_CLOSED) {
  	return SYSERR;
  }

  if (nbytes > ((INODEDIRECTBLOCKS * fsd.blocksz) - offset)) {
  	return SYSERR;
  }
  
  for (i = starting_block; i < INODEDIRECTBLOCKS; i++) {

	// allocate block if needed
	if (oft[fd].in.blocks[i] == 0) {
		next_available_block = get_next_available_block();

		if (next_available_block == SYSERR) {
			return SYSERR; // no free blocks available
		}

		oft[fd].in.blocks[i] = next_available_block;
	}

	if (nbytes - write_length > fsd.blocksz) {
		write_size = fsd.blocksz;
		
		if (i == starting_block) {
			write_size = fsd.blocksz - block_offset;
		}
	}
	else {
		write_size = nbytes - write_length;

		if (i == starting_block) {
			if (fsd.blocksz - block_offset > nbytes) {
				write_size = nbytes;
			}
			else {
				write_size = fsd.blocksz - block_offset;
			}
		}
	}

  	bs_bwrite(oft[fd].in.device, oft[fd].in.blocks[i], block_offset, buf + write_length, write_size);
	write_length += write_size;
	block_offset = 0;

	if (write_length >= nbytes) {
		break;
	}
  }

  oft[fd].fileptr += write_length;
  oft[fd].in.size += write_length;
  _fs_put_inode_by_num(dev0, oft[fd].in.id, &oft[fd].in);

  return write_length;
}

int fs_link(char *src_filename, char* dst_filename) {
  
  int i = 0, src_inode = -1, slot = -1;
  inode_t tmp_in;

  if (strlen(src_filename) == strlen(dst_filename) && 
	memcmp(src_filename, dst_filename, strlen(src_filename)) == 0) {
  	
	return SYSERR; // duplicate filename
  }

  for (i = 0; i < DIRECTORY_SIZE; i++) {
    	if (fsd.root_dir.entry[i].inode_num != EMPTY) {

		if (strlen(src_filename) == strlen(fsd.root_dir.entry[i].name) && 
			memcmp(src_filename, fsd.root_dir.entry[i].name, strlen(src_filename)) == 0) {
		
			src_inode = fsd.root_dir.entry[i].inode_num;
		}
		else if (strlen(dst_filename) == strlen(fsd.root_dir.entry[i].name) && 
			memcmp(dst_filename, fsd.root_dir.entry[i].name, strlen(dst_filename)) == 0) {
		
			return SYSERR; // duplicate filename
		}
	}
	else if (fsd.root_dir.entry[i].inode_num == EMPTY && slot == -1) {
		slot = i;
	}

  }

  if (src_inode == -1 || slot == -1) {
	return SYSERR;
  }
  else {
  	fsd.root_dir.entry[slot].inode_num = src_inode;
	memcpy(fsd.root_dir.entry[slot].name, dst_filename, strlen(dst_filename));
  	fsd.root_dir.numentries = fsd.root_dir.numentries + 1;
	_fs_get_inode_by_num(dev0, src_inode, &tmp_in);
	tmp_in.nlink = tmp_in.nlink + 1;
	_fs_put_inode_by_num(dev0, src_inode, &tmp_in);
	update_oft_entry(src_inode);
  }
	
  return OK;
}

int fs_unlink(char *filename) {
  
  int i = 0, j = 0;
  inode_t tmp_in;

  for (i = 0; i < DIRECTORY_SIZE; i++) {
    	if (fsd.root_dir.entry[i].inode_num != EMPTY && 
		strlen(filename) == strlen(fsd.root_dir.entry[i].name) && 
		memcmp(filename, fsd.root_dir.entry[i].name, strlen(filename)) == 0) {
		
		_fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &tmp_in);
		
		if (tmp_in.nlink > 1) {
			tmp_in.nlink = tmp_in.nlink - 1;
			_fs_put_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &tmp_in);
			update_oft_entry(fsd.root_dir.entry[i].inode_num);
		}
		else if (tmp_in.nlink == 1) {
			tmp_in.id = EMPTY;
			tmp_in.type = 0;
			tmp_in.nlink = 0;
			tmp_in.device = 0;
			tmp_in.size = 0;

			for (j = 0; j < INODEDIRECTBLOCKS; j++) {
				if (tmp_in.blocks[j] != 0) {
					fs_clearmaskbit(tmp_in.blocks[j]);
					tmp_in.blocks[j] = 0;
				}
			}
			_fs_put_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &tmp_in);
		}
		else {
			return SYSERR;
		}
			
		delete_oft_entry(filename);
		fsd.root_dir.entry[i].inode_num = EMPTY;
		memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
  		fsd.root_dir.numentries = fsd.root_dir.numentries - 1;
	
		return OK;
	}
  }
 
  return SYSERR;
}

#endif /* FS */
