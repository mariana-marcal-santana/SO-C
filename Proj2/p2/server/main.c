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

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "clients_manager.h"

sem_t semaphore_sessions;
pthread_mutex_t mutex_clients ;

struct Client all_clients[MAX_SESSION_COUNT];

void *wait_for_requests(void *arg) {
  struct Client *client = (struct Client *)arg;
  
  while (1) {
    //Open request pipe to read request
    int fd_request = open(client->path_request, O_RDONLY);
    if (fd_request == -1) {
      fprintf(stderr, "Error opening request pipe\n");
      return NULL;
    }
    
    char buffer_request[530];

    if (read(fd_request, buffer_request, 530) == -1) {
      fprintf(stderr, "Error reading from request pipe\n");
      return NULL;
    }

    if (close(fd_request) == -1) {
      fprintf(stderr, "Error closing request pipe\n");
      return NULL;
    }

    int flag_exit = 0;
    char op_code_char[2];
    strncpy(op_code_char, buffer_request, 1);
    op_code_char[1] = '\0';
    int op_code = atoi(op_code_char);

    switch (op_code) {
      case 2: // ems_quit
                                     
        printf("ems_quit\n");
        flag_exit = 1;

        if (unlink(client->path_request) == -1) {
            perror("Error unlinking request pipe");
            return NULL;
        }
        if (remove(client->path_response) == -1) {
          perror("Error unlinking response pipe\n");
          return NULL;
        }

        remove_Client(all_clients, client->id_session);
        if (sem_post(&semaphore_sessions) == -1) {
          fprintf(stderr, "Error posting semaphore\n");
          return NULL;
        }
        break;
      case 3: // ems_create
        break;
      case 4: // ems_reserve
        break;
      case 5: // ems_show
        break;
      case 6: // ems_list_events
        break;
    }
    if (flag_exit == 1) { break; }
  }
  pthread_exit(EXIT_SUCCESS);
}
  
  
void *wait_for_requests_session(void *arg) {
  
  set_list_Clients(all_clients);

  if (pthread_mutex_init(&mutex_clients, NULL) != 0) {
    fprintf(stderr, "Failed to initialize mutex\n");
    return NULL;
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
    //Parse request
    char request_type[2]; // Change the size to 2
    strncpy(request_type, buffer_request, 1); // Copy the first character from buffer to request_type
    request_type[1] = '\0'; // Add null terminator to request_type
 
    //Verify request type
    if (request_type[0] == '1') {               //////////NAO SERA NECESSARIO REALIZAR ESTA VERIFICACAO 
      fprintf (stderr, "Request session\n");
      int free_id_session = get_free_index(all_clients);
      
      char client_request[41], client_response[41];
      strncpy(client_request, buffer_request + 2, 40);
      client_request[41] = '\0';  
      strncpy(client_response, buffer_request + 42, 40);
      client_response[41] = '\0';

      char buffer_response[2];
      snprintf(buffer_response, 2, "%d", free_id_session);

      //Set Client
      set_Client(all_clients, free_id_session, client_request, client_response) ;
      pthread_create(&all_clients[free_id_session].thread, NULL, wait_for_requests, &all_clients[free_id_session]) ;

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
  //Create register FIFO to receive requests to register new clients
  if (mkfifo(path_register_FIFO, 0777) == -1) {
    fprintf(stderr, "Failed to create register FIFO\n");
    return 1;
  }   

  //Start the main thread to wait for new clients
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