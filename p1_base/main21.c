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
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>



int main(int argc, char *argv[]) {

    unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
    unsigned int max_processes = 1;
    
    // Check if a delay is provided
    if (argc > 3) {
        char *endptr;
        unsigned long int delay = strtoul(argv[3], &endptr, 10);

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

    fflush(stdout);

    // Set the path to the provided directory
    struct dirent *entry;
    char dirPath[MAX_PATH_LENGTH];
    getcwd(dirPath, sizeof(dirPath));
    strcat(dirPath,"/");
    strcat(dirPath,argv[1]);
    strcat(dirPath,"/");
    

    key_t key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Criar a memória compartilhada
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
      perror("shmget");
      exit(EXIT_FAILURE);
    }

    // Anexar a memória compartilhada ao espaço de endereçamento do processo
    sem_t *semaphore = (sem_t *)shmat(shmid, NULL, 0);
    if (semaphore == (sem_t *)(-1)) {
      perror("shmat");
      exit(EXIT_FAILURE);
    }

    // Create a semaphore
    if (sem_init(semaphore, 1 , max_processes) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    // Read the directory
    while ((entry=readdir(dir) )!= NULL) {

        if (strcmp(entry->d_name + strlen(entry->d_name) - 5, ".jobs") == 0) {

            if (sem_wait(semaphore) == -1) {
                perror("Error waiting on semaphore");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork() ;

            if (pid == -1){
                fprintf(stderr, "Error creating child process\n");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0) {
                // Child process
                char currentPath[MAX_PATH_LENGTH];
                char currentPath2[MAX_PATH_LENGTH];
        
                // Set the path to the file
                strcpy(currentPath, dirPath);
                strcat(currentPath, entry->d_name);

                int saved_stdin = dup(STDIN_FILENO);
                int saved_stdout = dup(STDOUT_FILENO);

                // Open the file
                int fd_input = open(currentPath, O_RDONLY);
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

                // Open the output file
                int fd_output = open(currentPath2, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
                if (fd_output == -1) {
                    perror("Failed to open output file");
                    fprintf(stderr, "File name: %s\n", output_filename);
                    close(fd_input);
                    continue;
                }

                // Redirect the standard input and output
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
                // Initialize the event management system
                if (ems_init(state_access_delay_ms)) {
                    fprintf(stderr, "Failed to initialize EMS\n");
                    return 1;
                }

                fflush(stdout);

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
                        fprintf(stdout,"Waiting...\n");
                        ems_wait(delay);
                        }
                        break;

                    case CMD_INVALID:
                        fprintf(stderr, "Invalid command. See HELP for usage\n");
                        break;

                    case CMD_HELP:
                        // Display help information
                        fprintf(stdout,
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
                        exitFlag = 1;
                        break;
                    }
                    fflush(stdout);
                }

                // Restore the standard input and output
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

                fflush(stdout);

                sem_post(semaphore);

                shmdt(semaphore);
                // Close the files
                close(fd_input);
                close(fd_output);
                exit(EXIT_SUCCESS);
            }
        }
    }
    pid_t wpid;
    int status ;
    while ((wpid = waitpid(-1, &status, 0)) != -1){
        if (WIFEXITED(status)){
            continue;   
        }
        else{
            printf("Child %d terminated abnormally\n", wpid);
        }
    }
    sem_destroy(semaphore);
    closedir(dir);
    return 0;
}



