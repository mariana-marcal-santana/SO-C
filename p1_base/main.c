#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "dirent.h"

#include <sys/stat.h>
#include <fcntl.h>


#define MAX_PATH_LENGTH 256

void processJobsDirectory(const char *directoryPath);
void handleInputFile(const char *filePath);




int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  // Check if a directory path is provided
  if (argc > 1) {
    processJobsDirectory(argv[1]);
  }

  // Check if a delay value is provided
  if (argc > 2) {
    char *endptr;
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }
    state_access_delay_ms = (unsigned int)delay;
  }

  // Initialize the event management system
  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  // Main command processing loop
  while (1) {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    printf("> ");
    fflush(stdout);

    switch (get_next(STDIN_FILENO)) {
      case CMD_CREATE:
        // Process create command
        if (parse_create(STDIN_FILENO, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid CREATE command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }
        break;

      case CMD_RESERVE:
        // Process reserve command
        num_coords = parse_reserve(STDIN_FILENO, MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid RESERVE command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }
        break;

      case CMD_SHOW:
        // Process show command
        if (parse_show(STDIN_FILENO, &event_id) != 0) {
          fprintf(stderr, "Invalid SHOW command. See HELP for usage\n");
          continue;
        }

        if (ems_show(event_id)) {
          fprintf(stderr, "Failed to show event\n");
        }
        break;

      case CMD_LIST_EVENTS:
        // Process list events command
        if (ems_list_events()) {
          fprintf(stderr, "Failed to list events\n");
        }
        break;

      case CMD_WAIT:
        // Process wait command
        if (parse_wait(STDIN_FILENO, &delay, NULL) == -1) {
          fprintf(stderr, "Invalid WAIT command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }
        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        // Display help information
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n");
        break;

      case CMD_BARRIER:  // Not implemented
      case CMD_EMPTY:
        break;

      case EOC:
        // Terminate the program
        ems_terminate();
        return 0;
    }
  }
}

void processJobsDirectory(const char *directoryPath) {
  DIR *dir = opendir(directoryPath);

  if (dir == NULL) {
    fprintf(stderr, "Invalid path\n");
    exit(1);
  }

  struct dirent *entry;
  char currentPath[MAX_PATH_LENGTH];

  while ((entry = readdir(dir)) != NULL) {
    getcwd(currentPath, sizeof(currentPath));

    // Check if the file has the .jobs extension
    if (strcmp(entry->d_name + strlen(entry->d_name) - 5, ".jobs") == 0) {
      // Build the full path to the input file
      strcat(currentPath, "/jobs/");
      strcat(currentPath, entry->d_name);

      // Process the input file
      handleInputFile(currentPath);
    }
  }

  closedir(dir);
}

void handleInputFile(const char *filePath) {
  int fd_input = open(filePath, O_RDONLY);

  int saved_stdin = dup(STDIN_FILENO);

  if (fd_input == -1) {
    perror("Failed to open input file");
    fprintf(stderr, "File name: %s\n", filePath);
    return;
  }

  // Redirect stdin to the input file
  if (dup2(fd_input, STDIN_FILENO) == -1) {
    perror("Failed to redirect stdin");
    close(fd_input);
    return;
  }

  // Process the input file
  main(0, NULL);
  
  // Close the file descriptor
  close(fd_input);

  dup2(saved_stdin, STDIN_FILENO);

}
