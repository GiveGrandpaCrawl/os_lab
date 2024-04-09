/*
 * dynamic memory allocation
 * update time: 2024-4-9
 * author: feng
 * version: 1.3
 * update log:
 * v1.0 -- structs, init(), alloc() and free()
 * v1.1 -- coalesce_blocks(), for reclaiming fragment
 * v1.2 -- alignment, size is the size of dheader(16)
 * v1.3 -- check if the free address is valid
 */

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define ALIGNMENT_SIZE 16 // 16B alignment

/*
 * Structure representing a header for a memory block in the dynamic memory heap.
 * Each block contains its information and a pointer to the next block.
 */
struct dheader {
  unsigned data_size;       // Size of the memory block (excluding header).
  unsigned used;            // Flag indicating whether the memory block is used or free.
  struct dheader *next;     // Pointer to the next memory block.
};

#define HEADER_SIZE 16 // size of dheader

/*
 * Structure representing the dynamic memory heap.
 * Contains a spinlock for synchronization and a pointer to the first memory block.
 */
struct {
  struct spinlock lock;     // Spinlock for synchronization.
  struct dheader *start;    // Pointer to the first memory block.
} dmem;

/*
 * Initializes the dynamic memory heap.
 * Sets up the spinlock and initializes the heap with a single block of size HEAP_SIZE.
 */
void
dinit()
{
  initlock(&dmem.lock, "dmem");
  
  // Allocate memory for the first block.
  dmem.start = (struct dheader*)(PHYSTOP - HEAP_SIZE);
  
  // Initialize the first block.
  dmem.start->data_size = HEAP_SIZE - HEADER_SIZE;
  dmem.start->used = 0; // Initially, the block is not used.
  dmem.start->next = 0; // No next block initially.
}

/*
 * dalloc function allocates a memory block of specified size from the dynamic memory heap.
 * If a suitable block is found in the heap, it is allocated and returned to the caller.
 * If no suitable block is found, returns NULL.
 */
void *
dalloc(unsigned size)
{
  // Calculate the total size required, including the size of the header.
  unsigned required_size = size + HEADER_SIZE;
  
  // Round up the required size to the nearest multiple of ALIGNMENT_SIZE.
  required_size = (required_size + (ALIGNMENT_SIZE - 1)) & ~(ALIGNMENT_SIZE - 1);
  
  // Initialize pointer to traverse the heap.
  acquire(&dmem.lock);
  struct dheader *curr = dmem.start;

  // Traverse the heap to find a suitable block.
  while (curr != 0) {
    // Check if the current block is large enough to accommodate the requested size.
    if (!curr->used && curr->data_size >= required_size) {
      // Check if the current block can be split into two blocks.
      if (curr->data_size > required_size) {
        // Create a new block after the allocated block.
        struct dheader *new_block = (struct dheader*)((char*)curr + required_size);
        new_block->data_size = curr->data_size - required_size;
        new_block->next = curr->next;
        new_block->used = 0; // The new block is initially unused.
        
        // Update the size of the allocated block and adjust the next pointer.
        curr->data_size = (size + (ALIGNMENT_SIZE - 1)) & ~(ALIGNMENT_SIZE - 1);
        curr->next = new_block;
      }
      
      // Mark the current block as used.
      curr->used = 1;
      
      // Release the lock before returning the allocated block.
      release(&dmem.lock);
      
      // Return a pointer to the allocated memory block, skipping the header.
      return (void*)(curr + 1);
    }
    
    // Move to the next block in the heap.
    curr = curr->next;
  }
  
  release(&dmem.lock);
  return 0;
}

/*
 * Coalesce adjacent free blocks in the dynamic memory heap.
 * This function merges contiguous free blocks into a single larger free block.
 */
void
coalesce_blocks()
{
  // Start from the beginning of the heap.
  struct dheader *curr = dmem.start;
  
  // Traverse the heap to coalesce adjacent free blocks.
  while (curr != 0 && curr->next != 0) {
    // If the current block and its next block are both free, merge them.
    if (!curr->used && !curr->next->used) {
      // Merge the next block into the current block.
      curr->data_size += curr->next->data_size + HEADER_SIZE;
      curr->next = curr->next->next;
    } else {
      // Move to the next block.
      curr = curr->next;
    }
  }
}

/*
 * dfree function deallocates a memory block previously allocated by dalloc.
 * It marks the specified memory block as free for reuse.
 */
void
dfree(void *addr)
{
  // Calculate the header address corresponding to the given data address.
  struct dheader *block_header = (struct dheader*)addr - 1;

  // Check if the address is properly aligned.
  if (((uint64)block_header) % ALIGNMENT_SIZE != 0) {
    panic("dfree: Address is not properly aligned.");
  }

  // Initialize pointer to traverse the heap.
  acquire(&dmem.lock);
  struct dheader *curr = dmem.start;

  // Traverse the heap to find the header with the given address.
  while (curr != 0) {
    // Check if the current header's address matches the given block header's address.
    if (curr == block_header) {
      // Mark the block as unused.
      curr->used = 0;

      // Coalesce adjacent free blocks if possible.
      coalesce_blocks();

      // Release the lock after deallocating the memory block.
      release(&dmem.lock);

      return;
    }
    // Move to the next header in the heap.
    curr = curr->next;
  }

  release(&dmem.lock);
  // If no header with the given address is found, panic.
  panic("dfree: Invalid address.");
}
