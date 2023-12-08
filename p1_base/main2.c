#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>

#include "constants.h"
#include "ems_operations.h"
#include "operations.h"
#include "parser.h"
#include "dirent.h"

int main(int argc, char *argv[]) {

    unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
    unsigned int max_proc = MAX_PROC ;
    
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
  
    // Set the maximum number of processes
    if (argc > 2) {
        char *endptr;
        unsigned long int proc = strtoul(argv[2], &endptr, 10);

        if (*endptr != '\0') {
        fprintf(stderr, "Invalid number of processes\n");
        return 1;
        }
        max_proc = (unsigned int)proc;
    }


    // Open the directory
    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        fprintf(stderr, "Invalid path\n");
        return 1;
    }

    fflush(stdout);

    // Set the path to the provided directory
    char dirPath[MAX_PATH_LENGTH];
    getcwd(dirPath, sizeof(dirPath));
    strcat(dirPath,"/");
    strcat(dirPath,argv[1]);
    strcat(dirPath,"/");
    
    // Create a key
    key_t key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create a shared memory segment
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
      perror("shmget");
      exit(EXIT_FAILURE);
    }

    // Link shared memory to the process address space
    sem_t *semaphore = (sem_t *)shmat(shmid, NULL, 0);
    if (semaphore == (sem_t *)(-1)) {
      perror("shmat");
      exit(EXIT_FAILURE);
    }   

    // Create a semaphore
    if (sem_init(semaphore, 1 , max_proc) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    // Read the directory
    struct dirent *entry;
    while ((entry = readdir(dir))!= NULL) {

        if (strcmp(entry->d_name + strlen(entry->d_name) - 5, ".jobs") == 0) {

            // Wait for a process to finish
            if (sem_wait(semaphore) == -1) {
                perror("Error waiting on semaphore");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("Failed to fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0) {
                     
                // Child process
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
                ems_process(fd_input);
                // Restore the standard input and output
                redirectStdinStdout(fd_input, fd_output, saved_stdin, saved_stdout, "STD");

                sem_post(semaphore);
                shmdt(semaphore);

                // Close the files
                close(fd_input);
                close(fd_output);

                exit(EXIT_SUCCESS);
            }
        }
    }
    // Wait for all the processes to finish
    while (wait(NULL) != -1) {
        continue;
    }
    sem_destroy(semaphore);
    closedir(dir);
    return 0;
}



