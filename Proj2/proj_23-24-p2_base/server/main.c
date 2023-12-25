#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>







#include "common/constants.h"
#include "common/io.h"
#include "operations.h"




void *wait_for_requests_session(void *arg) {

  int session_id = 0;
  char *path_register_FIFO = (char *)arg;

  sem_t semaphore ;

  if (sem_init(&semaphore, 0, MAX_SESSION_COUNT) == -1) {
    fprintf(stderr, "Failed to initialize semaphore\n");
    return NULL;
  }  
  while(1){
    sem_wait(&semaphore); 
    int fd_register = open(path_register_FIFO, O_RDONLY);
    if (fd_register == -1) {
      fprintf(stderr, "Error opening register pipe\n");
      return NULL;
    }

    char buffer[82] ;
    ssize_t bytes_read = read(fd_register, buffer, 82);
    if (bytes_read == -1) {
      fprintf(stderr, "Error reading from register pipe\n");
      return NULL;
    }

    if (buffer[0] == '1') {
      fprintf(stderr, "Received request: %s\n", buffer);
    }

    if (close(fd_register) == -1) {
      fprintf(stderr, "Error closing register pipe\n");
      return NULL;
    }
    break;
  }
}


int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }
    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  char* path_register_FIFO = malloc(strlen(argv[1]) + 1);
  strcpy(path_register_FIFO, argv[1]);

  if (mkfifo(path_register_FIFO, 0777) == -1) {
    fprintf(stderr, "Failed to create register FIFO\n");
    return 1;
  }   


  pthread_t requests_SESSIONS ;     
  char *arg = malloc(strlen(path_register_FIFO) + 1);
  strcpy(arg, path_register_FIFO);

  if (pthread_create(&requests_SESSIONS, NULL, wait_for_requests_session, arg)) {
    fprintf(stderr, "Failed to create thread\n");
    return 1;
  }

  //TODO: Intialize server, create worker threads

  while (1) {
    //TODO: Read from pipe
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  ems_terminate();
  free(arg);
  free(path_register_FIFO);
}