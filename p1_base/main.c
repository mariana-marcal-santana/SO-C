#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "constants.h"
#include "ems_operations.h"
#include "parser.h"
#include "dirent.h"

int main(int argc, char *argv[]) {

  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  // Check if a delay is provided
  if (argc > 2) {
    char *endptr;
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }
    state_access_delay_ms = (unsigned int)delay;
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

  fflush(stdout);

  // Read the directory
  while ((entry = readdir(dir)) != NULL) {
    char currentPathIn[MAX_PATH_LENGTH];
    char currentPathOut[MAX_PATH_LENGTH];

    // Set the path to the file
    strcpy(currentPathIn, dirPath);
    strcat(currentPathIn, entry->d_name);

    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    // Open the file
    int fd_input = open(currentPathIn, O_RDONLY);
    if (fd_input == -1) {
        perror("Failed to open input file");
        fprintf(stderr, "File name: %s\n", currentPathIn);
        continue;
    }

    // Set the output file name
    char output_filename[strlen(entry->d_name) + 1];
    strcpy(output_filename, entry->d_name);
    strncpy(output_filename + strlen(output_filename) - 5, ".out", 5);
    output_filename[strlen(entry->d_name)] = '\0';
    strcpy(currentPathOut, dirPath);
    strcat(currentPathOut, output_filename);

    // Open the output file
    int fd_output = open(currentPathOut, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    if (fd_output == -1) {
        perror("Failed to open output file");
        fprintf(stderr, "File name: %s\n", output_filename);
        close(fd_input);
        continue;
    }

    // Initialize the event management system
    if (ems_init(state_access_delay_ms)) {
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
    }

    // Redirect the standard input and output
    redirectStdinStdout(fd_input, fd_output, saved_stdin, saved_stdout, "FD");
    // Process the commands
    ems_process(fd_input, fd_output);
    // Restore the standard input and output
    redirectStdinStdout(fd_input, fd_output, saved_stdin, saved_stdout, "STD");

    // Close the files
    close(fd_input);
    close(fd_output);

      
  }
  closedir(dir);
  return 0;
}