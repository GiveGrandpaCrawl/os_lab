/*
 * test for "getprocs"
 * update time: 2024-3-31
 * author: feng
 * version: 1.3
 * update log:
 * v1.0 -- main()
 * v1.1 -- change the logic of fork() and wait()
 * v1.2 -- printf is not available, add function print_int() using write()
 * v1.3 -- sleep() for synchronization
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PROCESSES 3

void print(const char *s) {
  write(1, s, strlen(s));
}

// Helper function to convert integer to string and print it
void print_int(int num) {
  char buf[32];  // Assuming maximum integer length
  int i = 0;
  // Convert integer to string
  do {
    buf[i++] = '0' + num % 10;
  } while ((num /= 10) > 0);
  buf[i] = '\0';  // Ensure string termination
  // Print the string in reverse order
  while (i-- > 0) {
    write(1, &buf[i], 1);
  }
}

// Helper function to print the process count
void print_proc_count() {
  int count = getprocs();
  print("Active processes: ");
  print_int(count);
  print("\n");
}

int main() {
  int i, pid;

  // Init
  print("Init:\n");
  print_proc_count();

  // Fork several times
  for (i = 0; i < MAX_PROCESSES; i++) {
    pid = fork();
    if (pid < 0) {
      print("Fork failed\n");
      exit(1);
    } else if (pid == 0) { // Child process
      // Sleep for a while to observe active process count
      sleep(10);
      exit(0); // Child exits after sleeping
    } else { // Parent process
      // Print active process count after each fork
      print("After forking:\n");
      print_proc_count();
    }
  }

  // Wait for all child processes to exit
  for (i = 0; i < MAX_PROCESSES; i++) {
    wait(0);  // Wait for child processes to exit
  }

  // After waiting for all child processes
  print("After waiting:\n");
  print_proc_count();

  exit(0);
}
