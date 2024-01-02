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
  printf("Worker thread: %d\n", worker_thread->id_session);

  while (1) {
    
    pthread_cond_wait(&worker_thread->start_cond, &worker_thread->mutex);
   
    int reset = 1;
    // Open request pipe
    int fd_request = open(worker_thread->path_request, O_RDONLY);
    if (fd_request == -1) {
      fprintf(stderr, "Error opening request pipe\n");
      exit(EXIT_FAILURE);
    } 
    // Open response pipe
    int fd_response = open(worker_thread->path_response, O_WRONLY);
    if (fd_response == -1) {
      fprintf(stderr, "Error opening response pipe\n");
      exit(EXIT_FAILURE);
    }
    
    while (reset) {

      pthread_mutex_lock(&mutex_workers);
  
      int buffer_request[516] = {0};
      if (read(fd_request, buffer_request, sizeof(buffer_request)) == -1) {
        fprintf(stderr, "Error reading from request pipe\n");
        exit(EXIT_FAILURE);
      }                     

      int op_code = buffer_request[0];
      printf("op_code: %d\n", op_code);
      
      int return_type = 0;
      unsigned int event_id;
    
      switch (op_code) {
        case 2 : //ems_quit 
          pthread_mutex_unlock(&mutex_workers);
          printf("ems_quit\n");
          // Close request and response pipes
          if (close(fd_request) == -1) {
            fprintf(stderr, "Error closing request pipe\n");
            exit(EXIT_FAILURE);
          }
          if (close(fd_response) == -1) {
            fprintf(stderr, "Error closing response pipe\n");
            exit(EXIT_FAILURE);
          }

          //Unlink request and response pipes
          if (unlink(worker_thread->path_request) == -1) {
            fprintf(stderr, "Error unlinking request pipe\n");
            exit(EXIT_FAILURE);
          }
          if (unlink(worker_thread->path_response) == -1) {
            fprintf(stderr, "Error unlinking response pipe\n");
            exit(EXIT_FAILURE);
          }
          
          // Reset session
          reset_WorkerThread(worker_thread);
          perror("free = 1");
  
          if (sem_post(&semaphore_sessions) == -1) {
            fprintf(stderr, "Error posting semaphore\n");
            continue;
          }
          reset = 0;
          break;

        case 3 : //ems_create
          pthread_mutex_unlock(&mutex_workers);
          printf("ems_create\n");
          // Get event_id, num_rows and num_cols from buffer
          event_id = (unsigned int) buffer_request[1];
          size_t num_rows = (size_t) buffer_request[2];
          size_t num_cols = (size_t) buffer_request[3];

          return_type = ems_create(event_id, num_rows, num_cols);
    
          if (write(fd_response, &return_type, sizeof(int)) == -1) {
            fprintf(stderr, "Error writing to response pipe\n");
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
          
          // Send response
          if (write(fd_response, &return_type, sizeof(return_type)) == -1) {
            fprintf(stderr, "Error writing to response pipe\n");
            exit(EXIT_FAILURE);
          }
          break;

        case 5 : //show
          printf("show\n");
          pthread_mutex_unlock(&mutex_workers);
          // Get event_id from buffer
          event_id = (unsigned int) buffer_request[1];
          ems_show(fd_response, event_id);
          break;

        case 6 : //ems_list
          printf("ems_list\n");
          pthread_mutex_unlock(&mutex_workers);
          ems_list_events(fd_response);
          break;
      }
    }
  }
  exit(EXIT_SUCCESS); 
}


void *product_consumer_queue(void *arg) {
  
  set_list_WorkerThreads(all_worker_threads);

  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if (pthread_create(&all_worker_threads[i].thread, NULL, worker_thread, &all_worker_threads[i]) != 0) {
      fprintf(stderr, "Failed to create thread\n");
      return NULL;
    }
  }
  char *path_register_FIFO = (char *)arg;
  // Initialize semaphore
  if (sem_init(&semaphore_sessions, 0, MAX_SESSION_COUNT) == -1) {
    fprintf(stderr, "Failed to initialize semaphore\n");
    return NULL;
  }  
  // Open register and response pipes
  int fd_register = open(path_register_FIFO, O_RDONLY);
  if (fd_register == -1) {
    fprintf(stderr, "Error opening register pipe\n");
    return NULL;
  }
  int fd_response = open(path_register_FIFO, O_WRONLY);
  if (fd_response == -1) {
    fprintf(stderr, "Error opening response pipe\n");
    return NULL;
  }

  while(1) {
    // Read request from client
    char buffer_request[84];
    if (read(fd_register, buffer_request, 84) == -1) {
      fprintf(stderr, "Error reading from register pipe\n");
      break;
    }
    // Wait for new session
    sem_wait(&semaphore_sessions);

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
    
    //Send response
    if (write(fd_response, buffer_response, 2) == -1) {
      fprintf(stderr, "Error writing to response pipe\n");
      break;
    }
    
    //Unlock session
    printf("Unlock session\n");
    pthread_cond_signal(&free_worker_thread->start_cond);
  }

  //Close register and response pipes
  if (close(fd_register) == -1) {
    fprintf(stderr, "Error closing register pipe\n");
    return NULL;
  }
  if (close(fd_response) == -1) {
    fprintf(stderr, "Error closing response pipe\n");
    return NULL;
  }

  return NULL;
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

  if (pthread_join(requests_SESSIONS, NULL)) {
    fprintf(stderr, "Failed to join thread\n");
    return 1;
  }

  ems_terminate();
  if (unlink(path_register_FIFO) == -1) {
    fprintf(stderr, "Failed to unlink register FIFO\n");
    return 1;
  }
  free(arg);
  free(path_register_FIFO);
}