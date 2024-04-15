/*
 * memory allocation test
 * update time: 2024-4-15
 * author: feng
 * version: 1.4
 * update log:
 * v1.0 -- basic test
 * v1.1 -- boundary and random test
 * v1.2 -- stress test
 * v1.3 -- overwrite test
 * v1.4 -- more stressful stress test
 */

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void basic_test() {
  printf("Entering basic_test...\n");
  // Allocate space of size 100
  void *ptr = dalloc(100);
  if (ptr != 0) {
    printf("Allocated address: %p\n", ptr);
    // Free the space
    dfree(ptr);
    printf("Basic test passed.\n\n");
  } else {
    printf("Basic test failed: unable to allocate memory.\n\n");
  }
}

void boundary_test() {
  printf("Entering boundary_test...\n");
  // Test Case 1: Allocate Very Large Memory
  void *large_ptr = dalloc(HEAP_SIZE - 128);
  if (large_ptr != 0) {
    printf("Allocated very large memory.\n");
    dfree(large_ptr);
  } else {
    printf("Failed to allocate very large memory.\n");
    printf("Boundary test failed.\n\n");
    return;
  }

  // Test Case 2: Allocate Memory Exceeding Heap Size
  void *exceed_ptr = dalloc(HEAP_SIZE);
  if (exceed_ptr == 0) {
    printf("Allocated memory exceeded heap size.\n");
  } else {
    printf("Error: Allocated memory exceeded heap size.\n");
    dfree(exceed_ptr);
    printf("Boundary test failed.\n\n");
    return;
  }

  // Test Case 3: Allocate Very Small Memory
  void *small_ptr = dalloc(1);
  if (small_ptr != 0) {
    printf("Allocated very small memory.\n");
    dfree(small_ptr);
  } else {
    printf("Failed to allocate very small memory.\n");
    printf("Boundary test failed.\n\n");
    return;
  }

  printf("Boundary test passed.\n\n");
}

// Custom pseudo-random number generator
static unsigned long int next = 1;

int rand() {
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

void random_test() {
  printf("Entering random_test...\n");

  int passed = 0;
  int failed = 0;
  void *ptr_array[10] = {0}; // Array to store allocated addresses

  // Seed the pseudo-random number generator
  srand(1000);

  // Allocate memory
  for (int i = 0; i < 10; ++i) {
    // Generate random size for memory allocation (between 1 and 1000)
    unsigned size = (rand() % 100000) + 1;
    void *ptr = dalloc(size);
    if (ptr != 0) {
      printf("Allocated address: %p, size: %d\n", ptr, size);
      ptr_array[i] = ptr; // Store allocated address in array
      passed++;
    } else {
      printf("Failed to allocate memory.\n");
      failed++;
    }
  }

  // Free memory
  for (int i = 0; i < 10; ++i) {
    if (ptr_array[i] != 0) {
      dfree(ptr_array[i]);
      printf("Memory deallocated: %p\n", ptr_array[i]);
    }
  }

  printf("Random test completed. Passed: %d, Failed: %d.\n\n", passed, failed);
}

void stress_test() {
  printf("Entering stress_test...\n");

  int total_iterations = 255; // Total number of iterations
  int block_size = 65537; // 64KB + 1
  int passed = 0;
  int failed = 0;

  // Allocate memory blocks
  void *ptrs[total_iterations];
  for (int i = 0; i < total_iterations; ++i) {
    void *temp = dalloc(block_size);
    if (temp == 0) {
      failed++;
    }
    ptrs[i] = temp;
  }
  printf("The last allocated address: %p\n", ptrs[total_iterations - 1]);

  // Free memory blocks
  for (int i = 0; i < total_iterations; ++i) {
    if (ptrs[i] != 0) {
      dfree(ptrs[i]);
      passed++;
      if (passed % 10 == 0) {
        printf("%d tests passed.\n", passed);
      }
    }
  }

  printf("Stress test completed. \n%d allocations and deallocations attempted. \nPassed: %d, Failed: %d.\n\n", total_iterations, passed, failed);
}

// Structure to hold string data and size
typedef struct {
  char *str;
  unsigned size;
} StringData;

// String copy
char* my_strcpy(char *dest, const char *src) {
  char *temp = dest;
  while ((*dest++ = *src++))
    ;
  return temp;
}

// Compare two memory blocks
int cmp(const void *ptr1, const void *ptr2, unsigned size) {
  const unsigned char *p1 = (const unsigned char *)ptr1;
  const unsigned char *p2 = (const unsigned char *)ptr2;
  for (unsigned i = 0; i < size; ++i) {
    if (p1[i] != p2[i]) {
      return 0; // Blocks are different
    }
  }
  return 1; // Blocks are identical
}

void overwrite_test() {
  printf("Entering overwrite_test...\n");

  // Define array of string data
  StringData strings[] = {
    {"hello, world!", 13},
    {"I LOVE OS AND LOVE U", 20},
    {"Miss March 7th so beautiful, so cute, and so smart.", 51}
  };

  // Allocate memory blocks, write data, and compare with original
  int num_strings = sizeof(strings) / sizeof(strings[0]);
  char *ptrs[num_strings];
  int success = 1;
  for (int i = 0; i < num_strings; ++i) {
    // Allocate memory for string
    ptrs[i] = (char *)dalloc(strings[i].size + 1); // Add 1 for null terminator
    if (ptrs[i] == 0) {
      printf("Test failed: Failed to allocate memory for block %d\n\n", i + 1);
      success = 0;
    }
    // Copy string data
    my_strcpy(ptrs[i], strings[i].str);
  }

  for (int i = 0; i < num_strings; ++i) {
    printf("String %d: %s\n", i + 1, strings[i].str);
    // Compare data in memory block with original string
    if (!cmp(ptrs[i], strings[i].str, strings[i].size)) {
      printf("Test failed: Data in memory block %d is changed\n\n", i + 1);
      success = 0;
    }
  }

  if (success) {
    printf("Overwrite test passed.\n\n");
  }

  // Free allocated memory
  for (int i = 0; i < num_strings; ++i) {
    dfree(ptrs[i]);
  }
}

int
memtest(void)
{
  dinit();

  basic_test();
  boundary_test();
  random_test();
  stress_test();
  overwrite_test();

  return 0;
}
