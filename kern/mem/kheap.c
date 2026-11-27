#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/memory_manager.h>
#include "../conc/kspinlock.h"
#include "../conc/ksemaphore.h"
#include <kern/cpu/cpu.h>

#define BELONGS_TO_BUDDY (-10)
#define AV_PAGES ((KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE)
struct sleeplock KHEAP_SLEEP_LOCK;

struct FreeBlock {
	uint32 start;
	uint32 size;

	LIST_ENTRY(FreeBlock) prev_next_info;
};

LIST_HEAD(FreeBlockList, FreeBlock);
struct FreeBlockList free_block_list;


int32 size_starts_from_va[AV_PAGES + 1];
struct FreeBlock free_block_arr[AV_PAGES+1];


struct FreeBlock* get_valid_start(uint32 size) {
	struct FreeBlock *it, *exact_fit = NULL, *worst_fit = NULL;
	// [    FREE    ] | BREAK
	LIST_FOREACH(it, &free_block_list, FreeBlock){
		if(it->size == size){
			exact_fit = it;
		}
		if(worst_fit == NULL || worst_fit->size < it->size){
			worst_fit = it;
		}
	}

	if(exact_fit != NULL) {
		return exact_fit;
	}
	if(worst_fit != NULL && worst_fit->size >= size){
		return worst_fit;
	}
	return NULL;
}

uint32 get_page_index(uint32 address){
	return (address + PAGE_SIZE - 1) / PAGE_SIZE;
}
/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [0] GIVEN FUNCTIONS
//==============================================
// [1] INITIALIZE KERNEL HEAP:
//==============================================
void kheap_init()
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		buddy_init();
		set_kheap_strategy(KHP_PLACE_CUSTOMFIT);
		kheapPageAllocStart = KERNEL_HEAP_START;
		kheapPageAllocBreak = KERNEL_HEAP_START;
	}
	//==================================================================================
	//==================================================================================
	LIST_INIT(&free_block_list);


	init_sleeplock(&KHEAP_SLEEP_LOCK, "kernel heap sleep lock");
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
/**************************************************/
/* BLOCK ALLOCATOR */
/**************************************************/

int32 get_page_ind(void* va){
	return (ROUNDDOWN((uint32) va, PAGE_SIZE) - KERNEL_HEAP_START) / PAGE_SIZE;
}

void* create_buddy_space()
{

	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [4] create_buddy_space
	//Your code is here
	//Comment the following line
//	panic("create_buddy_space() not implemented");
	void* va = kmalloc(PAGE_SIZE);
	if(va == NULL)return NULL;
	uint32 page_ind = get_page_ind(va);
	size_starts_from_va[page_ind] = BELONGS_TO_BUDDY;
	return va;
}

void  delete_buddy_space(void *va)
{

	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [5] delete_buddy_space
	//Your code is here
	//Comment the following line
//	panic("delete_buddy_space() not implemented");
	uint32 pg_ind = get_page_ind(va);
	size_starts_from_va[pg_ind] = 1;
	kfree(va);
}

/**************************************************/
/* PAGE ALLOCATOR */
/**************************************************/
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [6] kmalloc
	//Your code is here
	//Comment the following line

	// remaining blocking
	if(size == 0 || size > KERNEL_HEAP_MAX - KERNEL_HEAP_START){
		return NULL;
	}
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE - META_DATA_SIZE){
		return alloc_block(size);
		// panic("Dynamic allocator not implemented");
	}
	acquire_sleeplock(&KHEAP_SLEEP_LOCK);

		//	LIST_INSERT_HEAD(&list_to_add, &struct_element);
		int number_of_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
		int32 number_of_bytes = number_of_pages * PAGE_SIZE;
		struct FreeBlock* free_group = get_valid_start(number_of_bytes);
		if(free_group == NULL){
			// empty, no free page

			uint32 start = kheapPageAllocBreak;

			if(number_of_pages * PAGE_SIZE > KERNEL_HEAP_MAX - start) return NULL;

			kheapPageAllocBreak += number_of_pages * PAGE_SIZE;

			for(uint32 page_index = 0; page_index < number_of_pages; ++page_index){
				uint32 current_va = start + page_index * PAGE_SIZE;
				int32 alloc_done = alloc_page(ptr_page_directory, current_va, PERM_PRESENT | PERM_WRITEABLE, 1);
				if(alloc_done != 0){
					panic("Could not allocate the page");
				}
				uint32 *pg_table = NULL;
				struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, current_va, &pg_table);
				if(frame_info != NULL){
					frame_info->virtualAddress = current_va;
				}
			}
			uint32 pg_ind = get_page_ind((void*)start);
			size_starts_from_va[pg_ind] = number_of_pages;
			release_sleeplock(&KHEAP_SLEEP_LOCK);
			return (void*)start;
		}
		else if(free_group->size >= number_of_bytes){
			uint32 start = free_group->start;
			for(uint32 page_index = 0; page_index < number_of_pages; ++page_index){
				uint32 current_va = start + page_index * PAGE_SIZE;
				int32 alloc_done = alloc_page(ptr_page_directory, current_va, PERM_PRESENT | PERM_WRITEABLE, 1);
				if(alloc_done != 0){
					panic("Could not allocate the page");
				}
				uint32 *pg_table = NULL;
				struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, current_va, &pg_table);
				if(frame_info != NULL){
					frame_info->virtualAddress = current_va;
				}
			}
			int32 newStart = start + number_of_bytes;
			int32 newSize = free_group->size - number_of_bytes;

			LIST_REMOVE(&free_block_list, free_group);
			if(newSize > 0){
			    struct FreeBlock *rem = &free_block_arr[get_page_ind((void*)newStart)];
			    rem->start = newStart;
			    rem->size = newSize;
			    LIST_INSERT_HEAD(&free_block_list, rem);
			}

			uint32 pg_ind = get_page_ind((void*)start);
			size_starts_from_va[pg_ind] = number_of_pages;
			release_sleeplock(&KHEAP_SLEEP_LOCK);
			return (void*)start;
		}else{
			panic("fi 7aga 8alt, kernel kmalloc");
		}

	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #ALLOCATED PAGES) IS REQUIRED */

}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [7] kfree
	//Your code is here
	//Comment the following line
//	panic("kfree() not implemented");

	uint32 va_int = (uint32) virtual_address;
	if(virtual_address == NULL || va_int < kheapPageAllocStart || va_int >= kheapPageAllocBreak){
		panic("kfree: invalid address");
	}
	uint32 pg_ind = get_page_ind(virtual_address);
	uint32 number_of_pages = size_starts_from_va[pg_ind];
	if(number_of_pages == 0){
		panic("kfree: this is not start of a block or nothing stored from this va.");
		return;
	}
	acquire_sleeplock(&KHEAP_SLEEP_LOCK);
	if(number_of_pages == BELONGS_TO_BUDDY){
		free_block(virtual_address);
		release_sleeplock(&KHEAP_SLEEP_LOCK);
		return;
	}

	// loop from va to end
	// free each block
	// check if merge
	// check if heap break move required
	// update free list if needed
	for(uint32 page_index = 0; page_index < number_of_pages; ++page_index){
		uint32 current_va = va_int + page_index * PAGE_SIZE;

		uint32 *pg_table = NULL;
		struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, current_va, &pg_table);
		if(frame_info != NULL){
			frame_info->virtualAddress = 0;
		}
		unmap_frame(ptr_page_directory, current_va);
	}
	size_starts_from_va[pg_ind] = 0;

	// merge with before or after free blocks

	struct FreeBlock *it = NULL, *prev = NULL, *next = NULL;
			LIST_FOREACH(it, &free_block_list, FreeBlock) {
				if (it->start + it->size == va_int) {
					prev = it;
				} else if(va_int + number_of_pages * PAGE_SIZE == it->start){
					next = it;
				}
			}


	uint32 cur_start = va_int;
	uint32 cur_size = number_of_pages * PAGE_SIZE;
	if(prev != NULL){
		// needs merge with the one before it
		cur_start = prev->start;
		cur_size += prev->size;
		LIST_REMOVE(&free_block_list, prev);
	}
	if(next != NULL){
		cur_size += next->size;
		LIST_REMOVE(&free_block_list, next);
	}

	// check if it's the last segment
	if(cur_start + cur_size == kheapPageAllocBreak){
		kheapPageAllocBreak -= cur_size;
		release_sleeplock(&KHEAP_SLEEP_LOCK);
		return;
	}
	struct FreeBlock *cur = &free_block_arr[get_page_ind((void*)cur_start)];
	cur->size = cur_size;
	cur->start = cur_start;
	LIST_INSERT_HEAD(&free_block_list, cur);
	release_sleeplock(&KHEAP_SLEEP_LOCK);
	return;


	/*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #FREED PAGES) IS REQUIRED */

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [8] kheap_physical_address
	//Your code is here
	//Comment the following line
	panic("kheap_physical_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [9] kheap_virtual_address
	//Your code is here
	//Comment the following line
	panic("kheap_virtual_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().
void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'26.BONUS#1] KERNEL REALLOC - krealloc
	//Your code is here
	//Comment the following line
	panic("krealloc() is not implemented yet...!!");
}
