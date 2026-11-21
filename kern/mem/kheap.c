#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/memory_manager.h>
#include "../conc/kspinlock.h"
#include "../conc/ksemaphore.h"
#include <kern/cpu/cpu.h>

struct FreeBlock {
	uint32 start;
	uint32 size;

	LIST_ENTRY(FreeBlock) prev_next_info;
};

LIST_HEAD(FreeBlockList, FreeBlock);
struct FreeBlockList free_block_list;


struct FreeBlock* get_valid_start(uint32 size) {
	struct FreeBlock *it, exact_fit = NULL, worst_fit = NULL, max_start = NULL;
	// [    FREE    ] | BREAK
	LIST_FOREACH(it, &free_block_list, FreeBlock){
		if(it->size == size){
			exact_fit = it;
		}
		if(worst_fit == NULL || worst_fit->size < it->size){
			worst_fit = it;
		}
		if(max_start == NULL || max_start->start < it->start){
			max_start = it;
		}
	}

	if(exact_fit != NULL) {
		return exact_fit;
	}
	if(worst_fit != NULL){
		return worst_fit;
	}
	return max_start;
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
	LIST_INIT(&free_page_list);



}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
/**************************************************/
/* BLOCK ALLOCATOR */
/**************************************************/
void* create_buddy_space()
{

	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [4] create_buddy_space
	//Your code is here
	//Comment the following line
	panic("create_buddy_space() not implemented");
}

void  delete_buddy_space(void *va)
{

	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [5] delete_buddy_space
	//Your code is here
	//Comment the following line
	panic("delete_buddy_space() not implemented");
}

/**************************************************/
/* PAGE ALLOCATOR */
/**************************************************/
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [6] kmalloc
	//Your code is here
	//Comment the following line

	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE){
		panic("Dynamic allocator not implemented");
	}
	else {
		//	LIST_INSERT_HEAD(&list_to_add, &struct_element);
		int number_of_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
		struct FreePage* free_group = get_valid_start(number_of_pages);
		if(free_group == NULL){
			// empty, no free page

			uint32 start = kheapPageAllocBreak;
			kheapPageAllocBreak += number_of_pages * PAGE_SIZE;
			for(uint32 page_index = 0; page_index < number_of_pages; ++page_index){
				uint32 current_va = start + page_index * PAGE_SIZE;
				int32 alloc_done = alloc_page(ptr_page_directory, current_va, PERM_PRESENT | PERM_WRITEABLE, 0);
				if(alloc_done != 0){
					panic("Could not allocate the page");
				}
			}
			return (void*)start;
		}
		else if(free_group->size >= size){
			uint32 start = free_group->start;

		}
		else if(free_group->start + free_group->size == kheapPageAllocBreak){
			int need_push = size - free_group->size;
			kheapPageAllocBreak += need_push;

		}
	}


	kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #ALLOCATED PAGES) IS REQUIRED */

}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [7] kfree
	//Your code is here
	//Comment the following line
	panic("kfree() not implemented");

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
