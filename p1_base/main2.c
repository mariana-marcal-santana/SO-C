#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "dirent.h"

int main(int argc, char *argv[]) {

    unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
    unsigned int max_processes;

  // Check if a delay is provided
  if (argc > 3) {
    char *endptr;
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }
    state_access_delay_ms = (unsigned int)delay;
  }
  
  // 
  if (argc > 2) {
    char *endptr;
    unsigned long int proc = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0') {
      fprintf(stderr, "Invalid number of processes\n");
      return 1;
    }
    max_processes = (unsigned int)proc;
  }

  // Open the directory
  DIR *dir = opendir(argv[1]);
  if (dir == NULL) {
    fprintf(stderr, "Invalid path\n");
    return 1;
  }

  // Set the path to the provided directory
  struct dirent *entry;
  char dirPath[MAX_PATH_LENGTH];
  getcwd(dirPath, sizeof(dirPath));
  strcat(dirPath,"/");
  strcat(dirPath,argv[1]);
  strcat(dirPath,"/");

  while (max_processes != 0 && readdir(dir) != NULL) {

    max_processes--;
  }
}