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
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>


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
  unsigned *n_childs = (unsigned *)shmat(shmid, NULL, 0);
  if (n_childs == (unsigned *)(-1)) {
      perror("shmat");
      exit(EXIT_FAILURE);
  }
  *n_childs = 0;


  sem_t semaphore;
  if (sem_init(&semaphore, 0 , max_processes) == -1) {
      perror("sem_init");
      exit(EXIT_FAILURE);
  }
  fflush(stdout);

  // Read the directory
  while ((entry=readdir(dir) )!= NULL) {
    
    if (strcmp(entry->d_name + strlen(entry->d_name) - 5, ".jobs") == 0) {
      printf("1\n");
      sem_wait(&semaphore); // wait for a child to finish
      printf("2\n");
      if ( *n_childs < max_processes){
        
        pid_t pid = fork() ;

        if (pid == -1){
          fprintf(stderr, "Error creating child process\n");
          (*n_childs)--;
          exit(EXIT_FAILURE);
        }

        else if (pid == 0){
          
          __sync_fetch_and_add(n_childs, 1);
          sem_post(&semaphore); // release the semaphore
          
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

          ems_process(fd_input);

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

          // Close the files
          close(fd_input);
          close(fd_output);
          __sync_fetch_and_sub(n_childs, 1);
          exit(EXIT_SUCCESS);
        }
      }
      if ( *n_childs == max_processes){
        printf("3\n");
        wait(NULL);
        printf("4\n");
      }
    }
  }
  closedir(dir);
  return 0;
}