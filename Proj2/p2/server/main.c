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
#include <string.h> // Add this line for the strcmp function

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"


struct Client{
  int id_session ;
  char *path_request ;
  char *path_response ;
  pthread_t thread ;
};

struct Client all_clients[MAX_SESSION_COUNT] ;


void set_list_Clients(struct Client *clients){
  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    clients[i].id_session = -1 ;
    clients[i].path_request = NULL ;
    clients[i].path_response = NULL ;
  }
}

void set_Client(struct Client *clients, int id_session, char *path_request, char *path_response){
  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    if (clients[i].id_session == -1){
      clients[i].id_session = id_session ;
      clients[i].path_request = path_request ;
      clients[i].path_response = path_response ;
      break ;
    }
  }
}

void remove_CLient(struct Client *clients, int id_session){
  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    if (clients[i].id_session == id_session){
      clients[i].id_session = -1 ;
      clients[i].path_request = NULL ;
      clients[i].path_response = NULL ;
      break ;
    }
  }
}

int get_free_index(struct Client *clients){
  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    if (clients[i].id_session == -1){
      return i ;
    }
  }
  return -1 ;
}



void * wait_for_requests_operation(void *arg) {
  //TODO
}

void *wait_for_requests_session(void *arg) {
  
  set_list_Clients(all_clients) ;

  sem_t semaphore_sessions ;
  char *path_register_FIFO = (char *)arg;

  if (sem_init(&semaphore_sessions, 0, MAX_SESSION_COUNT) == -1) {
    fprintf(stderr, "Failed to initialize semaphore\n");
    return NULL;
  }  
  while(1){
    
    sem_wait(&semaphore_sessions); 
  
    //Open register pipe to read request
    int fd_register = open(path_register_FIFO, O_RDONLY);
    if (fd_register == -1) {
      fprintf(stderr, "Error opening register pipe\n");
      return NULL;
    }
    char buffer_request[82] ;
    ssize_t bytes_read = read(fd_register, buffer_request, 82);
    if (bytes_read == -1) {
      fprintf(stderr, "Error reading from register pipe\n");
      return NULL;
    }
    printf("buffer: %s\n", buffer_request);
    //Close register pipe
    if (close(fd_register) == -1) {
      fprintf(stderr, "Error closing register pipe\n");
      return NULL;
    }
    //Parse request
    char request_type[2]; // Change the size to 2
    strncpy(request_type, buffer_request, 1); // Copy the first character from buffer to request_type
    request_type[1] = '\0'; // Add null terminator to request_type
    fprintf(stderr,"request_type: %s\n", request_type);
    //Verify request type
    if (request_type[0] == '1') {
      fprintf (stderr, "Request session\n");
      char buffer_response[2];
      int free_id_session = get_free_index(all_clients) ;
      buffer_response[0] = free_id_session + '0' ;
      buffer_response[1] = '\0' ;
      all_clients[free_id_session].id_session = free_id_session ;
      char path_request[40] ;
      char path_response[40] ;
      /////////TODO

      set_Client(all_clients, free_id_session, path_request, path_response) ;
      /////////
      int fd_response = open(path_register_FIFO, O_WRONLY);
      if (fd_response == -1) {
        fprintf(stderr, "Error opening response pipe\n");
        return NULL;
      }
      fprintf(stderr, "buffer_response: %s\n", buffer_response);
      //Send response
      if (write(fd_response, buffer_response, 2) == -1) {
        fprintf(stderr, "Error writing to response pipe\n");
        return NULL;
      }

    
    }
    else if (request_type[0] == '0'){
      fprintf (stderr, "Request quit client\n");
      //TODO
    }
    
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
  //TODO: Read from pipe
  //TODO: Write new client to the producer-consumer buffer
  
  if (pthread_join(requests_SESSIONS, NULL)) {
    fprintf(stderr, "Failed to join thread\n");
    return 1;
  }

  //TODO: Close Server
  ems_terminate();
  free(arg);
  free(path_register_FIFO);
}