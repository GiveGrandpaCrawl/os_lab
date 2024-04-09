/*
 * memory allocation test for user, here call memtest() as a system call
 * update time: 2024-4-9
 * author: feng
 * version: 1.0
 * update log:
 * v1.0 -- main()
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
print(const char *s)
{
  write(1, s, strlen(s));
}

int main() {
  if (memtest() == 0) {
    print("Alloctest Success!\n");
  }
  else {
    print("Alloctest Failed!\n");
  }
  exit(0);
}