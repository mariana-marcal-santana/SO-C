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
#include <string.h> 
#include <stdbool.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "clients_manager.h"

sem_t semaphore_sessions;
pthread_mutex_t mutex_workers ;
struct Worker_Thread all_worker_threads[MAX_SESSION_COUNT];

void *worker_thread(void *arg){

  struct Worker_Thread *worker_thread = (struct Worker_Thread *)arg;
  
  int operational = 1 ;

  while (operational){
    
    pthread_cond_wait(&worker_thread->start_cond, &worker_thread->mutex);
    //printf("Worker thread %d started\n", worker_thread->id_session);
    int reset = 1;

    pthread_mutex_lock(&mutex_workers);
    while (reset) {
 
      int fd_request = open(worker_thread->path_request, O_RDONLY);
      if (fd_request == -1) {
        fprintf(stderr, "Error opening request pipe\n");
        exit(EXIT_FAILURE);
      }

      int buffer_request[600] = {0};
      ssize_t num_bytes = read(fd_request, buffer_request, sizeof(buffer_request));
      printf("num_bytes: %ld\n", num_bytes);
      for (int i = 0; i < 20; i++) {
        printf("buffer_request[%d]: %d\n", i, buffer_request[i]);
      }    

      if (close(fd_request) == -1) {
        fprintf(stderr, "Error closing request pipe\n");
        exit(EXIT_FAILURE);
      }

      int op_code = buffer_request[0];
      printf("op_code: %d\n", op_code);
      // Variables for switch-case
      int  fd_response, return_type;
    
      switch (op_code) {

        case 2 : //ems_quit 
          pthread_mutex_unlock(&mutex_workers);
          printf("ems_quit\n");
        
          if (unlink(worker_thread->path_request) == -1) {
            perror("Error unlinking request pipe");
            continue; 
          }
          if (unlink(worker_thread->path_response) == -1) {
            perror("Error unlinking response pipe\n");
            continue;
          }
          if (sem_post(&semaphore_sessions) == -1) {
            fprintf(stderr, "Error posting semaphore\n");
            continue;
          }
          reset_WorkerThread(worker_thread);
          reset = 0;
          break;

        case 3 : //ems_create
          pthread_mutex_unlock(&mutex_workers);
          printf("ems_create\n");

          // Get event_id, num_rows and num_cols from buffer
          unsigned int event_id = (unsigned int) buffer_request[1];
          size_t num_rows = (size_t) buffer_request[2];
          size_t num_cols = (size_t) buffer_request[3];
          printf("event_id: %d\n", event_id);
          printf("num_rows: %ld\n", num_rows);
          printf("num_cols: %ld\n", num_cols);

          return_type = ems_create(event_id, num_rows, num_cols);
    
          fd_response = open(worker_thread->path_response, O_WRONLY);
          if (fd_response == -1) {
            fprintf(stderr, "Error opening response pipe\n");
            exit(EXIT_FAILURE);
          }
          if (write(fd_response, &return_type, sizeof(int)) == -1) {
            fprintf(stderr, "Error writing to response pipe\n");
            exit(EXIT_FAILURE);
          }
          if (close(fd_response) == -1) {
            fprintf(stderr, "Error closing response pipe\n");
            exit(EXIT_FAILURE);
          }
          break;

        case 4 : //reserve
          pthread_mutex_unlock(&mutex_workers);
          printf("reserve\n");
          // Get event_id, num_seats, xs and ys from buffer
          event_id = (unsigned int) buffer_request[1];
          size_t num_seats = (size_t) buffer_request[2];
          size_t xs[MAX_RESERVATION_SIZE];
          size_t ys[MAX_RESERVATION_SIZE];
          for (size_t i = 0; i<num_seats; i++) {
            if (buffer_request[3 + i] == -1) {
              break;
            }
            xs[i] = (size_t) buffer_request[3 +i];
          }
          for (size_t i = 0; i<num_seats; i++) {
            if (buffer_request[4 + num_seats + i] == 0) {
              break;
            }
            ys[i] = (size_t) buffer_request[4 + num_seats + i];
          }

          return_type = ems_reserve(event_id, num_seats, xs, ys);
          // Open response pipe
          fd_response = open(worker_thread->path_response, O_WRONLY);
          if (fd_response == -1) {
            fprintf(stderr, "Error opening response pipe\n");
            exit(EXIT_FAILURE);
          }
          // Send response
          if (write(fd_response, &return_type, sizeof(return_type)) == -1) {
            fprintf(stderr, "Error writing to response pipe\n");
            exit(EXIT_FAILURE);
          }
          // Close response pipe
          if (close(fd_response) == -1) {
            fprintf(stderr, "Error closing response pipe\n");
            exit(EXIT_FAILURE);
          }
          break;

        case 5 : //show
          printf("show\n");
          // Get event_id from buffer
          event_id = (unsigned int) buffer_request[1];
          
          ems_show(worker_thread->path_response, event_id);

          pthread_mutex_unlock(&mutex_workers);
          break;

        case 6 : //ems_list
          printf("ems_list\n");
          
          ems_list_events(worker_thread->path_response);
          pthread_mutex_unlock(&mutex_workers);
          break;
      }
    }
  }
  exit(EXIT_SUCCESS); 
}


void *product_consumer_queue(void *arg) {
  
  set_list_WorkerThreads(all_worker_threads);

  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if(pthread_create(&all_worker_threads[i].thread, NULL, worker_thread, &all_worker_threads[i]) != 0) {
      fprintf(stderr, "Failed to create thread\n");
      return NULL;
    }
  }
  char *path_register_FIFO = (char *)arg;

  if (sem_init(&semaphore_sessions, 0, MAX_SESSION_COUNT) == -1) {
    fprintf(stderr, "Failed to initialize semaphore\n");
    return NULL;
  }  
  while(1) {
    
    sem_wait(&semaphore_sessions); 
  
    //Open register pipe to read request
    int fd_register = open(path_register_FIFO, O_RDONLY);
    if (fd_register == -1) {
      fprintf(stderr, "Error opening register pipe\n");
      return NULL;
    }
    char buffer_request[84];
    if (read(fd_register, buffer_request, 84) == -1) {
      fprintf(stderr, "Error reading from register pipe\n");
      return NULL;
    }
  
    //Close register pipe
    if (close(fd_register) == -1) {
      fprintf(stderr, "Error closing register pipe\n");
      return NULL;
    }
    
    fprintf (stderr, "Request session\n");
    
    //Get free session 
    struct Worker_Thread *free_worker_thread = get_free_worker_thread(all_worker_threads);
    
    char client_request[41], client_response[41];
    strncpy(client_request, buffer_request + 2, 40);
    client_request[41] = '\0';  
    strncpy(client_response, buffer_request + 42, 40);
    client_response[41] = '\0';

    //Set session as busy
    free_worker_thread->free = 0;
    free_worker_thread->path_request = client_request;
    free_worker_thread->path_response = client_response;
    
    //Create response
    char buffer_response[2];
    int response = free_worker_thread->id_session;
    sprintf(buffer_response, "%d", response); 
    buffer_response[1] = '\0';
    printf("buffer_response: %s\n", buffer_response);
    //Open response pipe to send response
    int fd_response = open(path_register_FIFO, O_WRONLY);
    if (fd_response == -1) {
      fprintf(stderr, "Error opening response pipe\n");
      return NULL;
    }

    //Send response
    if (write(fd_response, buffer_response, 2) == -1) {
      fprintf(stderr, "Error writing to response pipe\n");
      return NULL;
    }
    //Close response pipe
    if (close(fd_response) == -1) {
      fprintf(stderr, "Error closing response pipe\n");
      return NULL;
    }
    //Unlock session
    free_worker_thread->free = 1;  
    pthread_cond_signal(&free_worker_thread->start_cond);
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
  //Create register FIFO to receive requests to register new clients
  if (mkfifo(path_register_FIFO, 0777) == -1) {
    fprintf(stderr, "Failed to create register FIFO\n");
    return 1;
  }   

  //Start the main thread to wait for new clients
  pthread_t requests_SESSIONS ;     
  char *arg = malloc(strlen(path_register_FIFO) + 1);
  strcpy(arg, path_register_FIFO);

  if (pthread_create(&requests_SESSIONS, NULL, product_consumer_queue, arg)) {
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
  if (unlink(path_register_FIFO) == -1) {
    fprintf(stderr, "Failed to unlink register FIFO\n");
    return 1;
  }
  free(arg);
  free(path_register_FIFO);
}