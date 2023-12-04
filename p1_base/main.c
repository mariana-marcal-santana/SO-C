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

  // Read the directory
  while ((entry = readdir(dir)) != NULL) {
    
    char currentPath[MAX_PATH_LENGTH];
    char currentPath2[MAX_PATH_LENGTH];
      // Check if the file is a .jobs file
      if (strcmp(entry->d_name + strlen(entry->d_name) - 5, ".jobs") == 0) {
        strcpy(currentPath, dirPath);
        strcat(currentPath, entry->d_name);
        
        // Open the file
        int fd_input = open(currentPath, O_RDONLY);
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);

        if (fd_input == -1) {
            perror("Failed to open input file");
            fprintf(stderr, "File name: %s\n", currentPath);
            continue;
        }

        // Set the output file name
        char output_filename[strlen(entry->d_name) + 1];
        strcpy(output_filename, entry->d_name);
        strncpy(output_filename + strlen(output_filename) - 5, ".out", 5);
        output_filename[strlen(entry->d_name)] = '\0';
        strcpy(currentPath2, dirPath);
        strcat(currentPath2, output_filename);
        //printf("Output file: %s\n", currentPath2);

        // Open the output file
        int fd_output = open(currentPath2, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
        if (fd_output == -1) {
            perror("Failed to open output file");
            fprintf(stderr, "File name: %s\n", output_filename);
            close(fd_input);
            continue;
        }

        if (dup2(fd_output, STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout");
            close(fd_output);
            continue;
        }
        if (dup2(fd_input, STDIN_FILENO) == -1) {
            perror("Failed to redirect stdin");
            close(fd_input);
            continue;
        }
        //printf("Input file: %s\n", currentPath);
        // Initialize the event management system
        if (ems_init(state_access_delay_ms)) {
          fprintf(stderr, "Failed to initialize EMS\n");
          return 1;
        }

        // Main command processing loop
        int exitFlag = 0;
        while (!exitFlag) {
          
          unsigned int event_id, delay;
          size_t num_rows, num_columns, num_coords;
          size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

          switch (get_next(fd_input)) {
            
            case CMD_CREATE:
              // Process create command
              if (parse_create(fd_input, &event_id, &num_rows, &num_columns) != 0) {
                fprintf(stderr, "Invalid CREATE command. See HELP for usage\n");
                continue;
              }

              if (ems_create(event_id, num_rows, num_columns)) {
                fprintf(stderr, "Failed to create event\n");
              }
              break;

            case CMD_RESERVE:
              // Process reserve command
              num_coords = parse_reserve(fd_input, MAX_RESERVATION_SIZE, &event_id, xs, ys);

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
              if (parse_show(fd_input, &event_id) != 0) {
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
              if (parse_wait(fd_input, &delay, NULL) == -1) {
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
              close(fd_input);
              close(fd_output);
              exitFlag = 1;
              break;
          }
        }
        // Restore the standard input
        if (dup2(saved_stdin, STDIN_FILENO) == -1) {
            perror("Failed to restore stdin");
            close(fd_input);
            continue;
        }
        if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
            perror("Failed to restore stdout");
            close(fd_output);
            continue;
        }
      }
  }
  closedir(dir);
  return 0;
}

