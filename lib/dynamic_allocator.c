/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [0] GIVEN FUNCTIONS
//==================================
// [1] INITIALIZE BUDDY FREE LISTS:
//==================================
void buddy_init()
{
	//panic("buddy_init is not implemented yet");

	for (int l = BUDDY_LOWER_LEVEL; l <= BUDDY_UPPER_LEVEL; ++l)
	{
		LIST_INIT(&(freeBlockLists[l]));
	}
}
//====================================
// [2] PRINT SIZE OF FREE LISTS:
//====================================
void PrintFreeBlockLists()
{
	for (int i = BUDDY_LOWER_LEVEL; i <= BUDDY_UPPER_LEVEL; ++i)
	{
		cprintf("Level#%d: %d\n", i, LIST_SIZE(&freeBlockLists[i]));
	}
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] GET BLOCK SIZE:
//===========================
__inline__ uint32 get_block_size(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [1] get_block_size
	//Your code is here
	// va = 10 + va
	uint32 tempva = (uint32)va - META_DATA_SIZE;
    struct BlockMetaData *temp = (struct BlockMetaData*)((uint32)tempva);
    uint32 lvl = temp->level;
    return BLOCK_SIZE(lvl);
	// Comment the following line
	// panic("get_block_size() not implemented");

}
//===========================
// [2] ALLOCATE BLOCK:
//===========================


int32 neraestpoweroftwo(uint32 sz){
   for( uint32 i = BUDDY_LOWER_LEVEL;i<=BUDDY_UPPER_LEVEL;i++){
         uint32 temp = 1 << i;
         if(temp >= sz){
        	 return i;
         }
   }
   return -1;
}


void *alloc_block(uint32 requiredSize)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [2] alloc_block
	//Your code is here
	if(requiredSize == 0 || requiredSize > (1 << BUDDY_UPPER_LEVEL))return NULL;
	uint32 nsz = requiredSize + META_DATA_SIZE;
	if(nsz > (1 << BUDDY_UPPER_LEVEL))return NULL;
    int32 lvl = neraestpoweroftwo(nsz);
    if(lvl == -1)return NULL;
    // if there is a free block in the size i need
    if(!LIST_EMPTY(&freeBlockLists[lvl])){
           struct BlockMetaData* temp  = LIST_FIRST(&freeBlockLists[lvl]);
           LIST_REMOVE(&freeBlockLists[lvl],temp);
           temp->level = lvl;
           temp->is_free = 0;
           return (void*)((uint32)temp + META_DATA_SIZE);
    }else {
        int32 avaliable = -1;
    	for(int i = lvl;i<=BUDDY_UPPER_LEVEL;i++){
           if(!LIST_EMPTY(&freeBlockLists[i])){
        	   avaliable = i;
        	   break;
           }
    	}
    	if(avaliable == -1){
    		// get a new page and do your thing
    		// create a buddy space to get a new page
    		void *newva = create_buddy_space();
    		if(newva == NULL)return NULL;
    		avaliable = BUDDY_UPPER_LEVEL;
            struct BlockMetaData* temp  = (struct BlockMetaData*)newva;
            temp->level = BUDDY_UPPER_LEVEL;
            temp->is_free=1;
    		LIST_INSERT_TAIL(&freeBlockLists[BUDDY_UPPER_LEVEL],temp);

    		newva = newva + BLOCK_SIZE(BUDDY_UPPER_LEVEL);
    		temp = (struct BlockMetaData*)newva;
    		temp->level = BUDDY_UPPER_LEVEL;
    		temp->is_free=1;
    		LIST_INSERT_TAIL(&freeBlockLists[BUDDY_UPPER_LEVEL],temp);
    	}
    	// split the page until we match the size
        struct BlockMetaData* cur  = LIST_FIRST(&freeBlockLists[avaliable]);
        LIST_REMOVE(&freeBlockLists[avaliable],cur);
    	while(avaliable > lvl){
    		avaliable--;
    		cur->level = avaliable;
    		cur->is_free = 1;
    		struct BlockMetaData* right  = (struct BlockMetaData*)((uint32)cur + BLOCK_SIZE(avaliable));
    		right->level = avaliable;
    		right->is_free = 1;
    		LIST_INSERT_TAIL(&freeBlockLists[avaliable],right);
    	}
    	cur->level = lvl;
    	cur->is_free  = 0;
    	return (void *)((uint32)cur + META_DATA_SIZE);
    }
}

//===================================================
// [3] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [3] free_block
	//Your code is here
	if(va == NULL) return;

	uint32 block_address = (uint32)va - META_DATA_SIZE;
	struct BlockMetaData* block_metadata = (struct BlockMetaData*)(block_address);
	uint32 block_size = BLOCK_SIZE(block_metadata->level);

	if(block_metadata->is_free == 1) return;

	while(block_metadata->level < BUDDY_UPPER_LEVEL + 1) {
	    uint32 buddy_address = (block_address ^ BLOCK_SIZE(block_metadata->level));
		struct BlockMetaData* buddy_metadata = (struct BlockMetaData*)(buddy_address);

		if(buddy_metadata->is_free == 1 && block_metadata->level == buddy_metadata->level) {
		    LIST_REMOVE(&freeBlockLists[buddy_metadata->level], buddy_metadata);
			block_address = MIN(block_address, buddy_address);
			block_metadata = (struct BlockMetaData*)(block_address);
			block_metadata->level++;
		} else {
		    break;
		}
	}
	block_metadata->is_free = 1;
	if(block_metadata->level == BUDDY_UPPER_LEVEL + 1) {
	    delete_buddy_space((void *)block_address);
	} else {
    	LIST_INSERT_HEAD(&freeBlockLists[block_metadata->level], block_metadata);
	}
	//Comment the following line
	// panic("free_block() not implemented");
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//============================================== ===================================//

//===========================
// [1] REALLOCATE BLOCK:
//===========================
void *realloc_block(void* va, uint32 newSize)
{
	//TODO: [PROJECT'26.BONUS#1] KERNEL REALLOC - realloc_block
	//Your code is here
	//Comment the following line
	panic("realloc_block() Not implemented yet");
}
