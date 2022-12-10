
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include "common.h"
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct trans_table_t * get_trans_table(
		addr_t index, 	// Segment level index
		struct page_table_t * page_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [page_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	for (i = 0; i < page_table->size; i++) {
		// Enter your code here
		if(index == page_table->table[i].v_index) {
			return page_table->table[i].next_lv;
		}
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct trans_table_t * trans_table = NULL;
	trans_table = get_trans_table(first_lv, proc->seg_table);
	if (trans_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < trans_table->size; i++) {
		if (trans_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of trans_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			int physical_index = trans_table->table[i].p_index;

			*physical_addr = ((physical_index << OFFSET_LEN) | offset);
			printf("PID %d: virtual 0x%02x -> physical 0x%02x\n", proc->pid, virtual_addr, *physical_addr);
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */
	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE + 1:
		size / PAGE_SIZE; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?
	
	uint32_t *free_frame_physical_indexes = calloc(num_pages, sizeof(uint32_t));
	uint32_t free_frame_index = 0;

	for (uint32_t i = 0; i < NUM_PAGES; i++) {
		if(_mem_stat[i].proc == 0) {
			free_frame_physical_indexes[free_frame_index] = i;
			free_frame_index++;
		}
		if(free_frame_index == num_pages) {
			mem_avail = 1;
			break;
		}
	}

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
	if(mem_avail == 0 || proc->bp + PAGE_SIZE * num_pages > RAM_SIZE) {
		printf("PID %d: Not enough memory", proc->pid);
		return 0;
	}
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		for(uint32_t i = 0; i < free_frame_index; i++) {
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
			if(i == free_frame_index - 1) _mem_stat[free_frame_physical_indexes[i]].next = (int32_t)-1;
			else _mem_stat[free_frame_physical_indexes[i]].next = (int32_t) free_frame_physical_indexes[i + 1];
			_mem_stat[free_frame_physical_indexes[i]].proc = proc->pid;
			_mem_stat[free_frame_physical_indexes[i]].index = i;

			addr_t current_v_address = proc->bp + i * PAGE_SIZE;
			addr_t current_segment_v_index = get_first_lv(current_v_address);
			addr_t current_page_index = get_second_lv(current_v_address);

			// printf("PID: %d virtual index -> %d\n", proc->pid, current_v_address);
			// printf("PID: %d segment index (first level) -> %d\n", proc->pid, current_segment_v_index);
			// printf("PID: %d page index (second level) -> %d\n\n", proc->pid, current_page_index);

			struct trans_table_t* page_table = NULL;
			for(addr_t segment_index = 0; segment_index < proc->seg_table->size; segment_index++) {
				if(proc->seg_table->table[segment_index].v_index == current_segment_v_index) {
					page_table = proc->seg_table->table[segment_index].next_lv;
					break;
				}
			}

			if(page_table == NULL) {
				page_table = calloc(1, sizeof(struct trans_table_t));
				proc->seg_table->table[proc->seg_table->size].next_lv = page_table;
				proc->seg_table->table[proc->seg_table->size].v_index = current_segment_v_index;
				proc->seg_table->size++;
			}
			page_table->table[page_table->size].v_index = current_page_index;
			page_table->table[page_table->size].p_index = free_frame_physical_indexes[i];
			page_table->size++;
		}
	}
	proc->bp = proc->bp + num_pages * PAGE_SIZE;
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	bool check = true;
	addr_t current_address = address;
	while(check) {
		addr_t current_segment_v_index = get_first_lv(current_address);
		addr_t current_page_v_index = get_second_lv(current_address);

		struct trans_table_t* page_table = get_trans_table(current_segment_v_index, proc->seg_table);

		if(page_table == NULL) {
			pthread_mutex_unlock(&mem_lock);
			return 0;
		}

		addr_t current_page_index = 32;
		for(addr_t i = 0; i < page_table->size; i++) {
			if(page_table->table[i].v_index == current_page_v_index) {
				current_page_index = i;
				break;
			}
		}
		if (current_page_index == 32) {      // if we can't find the page
            pthread_mutex_unlock(&mem_lock); // bail out
			return 0;
        }

		addr_t physical_index = page_table->table[current_page_index].p_index;
		if(_mem_stat[physical_index].next != -1) check = true;
		else check = false;

		_mem_stat[physical_index].index = 0;
		_mem_stat[physical_index].next = -1;
		_mem_stat[physical_index].proc = 0;

		page_table->table[current_page_index].p_index = 32;
		page_table->table[current_page_index].v_index = 32;

		for(addr_t i = current_page_index; i < page_table->size - 1; i++) {
			page_table->table[i] = page_table->table[i + 1];
		}

		page_table->table[page_table->size].p_index = 32;
		page_table->table[page_table->size].v_index = 32;
		page_table->size--;
		if(page_table->size == 32) {
			for(addr_t i = 0; i < proc->seg_table->size; i++) {
				if(proc->seg_table->table[i].v_index == current_segment_v_index) {
					free(proc->seg_table->table[i].next_lv);

					for(addr_t j = i; j < proc->seg_table->size - 1; j++) {
						proc->seg_table->table[j] = proc->seg_table->table[j + 1];
					}
					proc->seg_table->size--;
					break;
				}
			}
		}
		current_address += PAGE_SIZE;
	}
	pthread_mutex_unlock(&mem_lock);
	return 1;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
        printf("PID: %d read at address 0x%x, got data 0x%02x\n", proc->pid, address, *data);
		return 0;
	}else{
		printf("PID: %d failed to read at address 0x%02x\n", proc->pid, address);
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
        printf("PID: %d wrote at address 0x%x, with data 0x%02x\n", proc->pid, address, data);
		return 0;
	}else{
        printf("PID: %d failed to write at address 0x%x, with data 0x%02x\n", proc->pid, address, data);
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05d-%05d - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


