#include "api.h"

#include <fcntl.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <sys/stat.h>
#include <sys/types.h>

char const* path_fifo_server;

int id_session ;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  
  path_fifo_server = server_pipe_path;

  //Create the paths
  size_t len_request =  strlen(req_pipe_path) + 1;
  char  *path_request = malloc(len_request);
  snprintf(path_request, len_request, "%s", req_pipe_path);

  size_t len_response = strlen(resp_pipe_path) + 1;
  char *path_response = malloc(len_response);
  snprintf(path_response, len_response, "%s", resp_pipe_path);
 

  //Connect to the server pipe
  int fd_server_resquest = open(server_pipe_path, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }

  //Create the pipes
  if(mkfifo(path_request, 0777) == -1){
    fprintf(stderr, "Error creating request pipe\n");
    return 1;
  }

  if(mkfifo(path_response, 0777) == -1){
    fprintf(stderr, "Error creating response pipe\n");
    return 1;
  }

  // Set the message to send to the server
  char buffer1[84], buffer_request[41], buffer_response[41];

  buffer1[0] = '1';
  buffer1[1] = '\0';

  strncpy(buffer_request, path_request, sizeof(buffer_request) - 1);
  buffer_request[sizeof(buffer_request) - 1] = '\0';  // Ensure null termination
  if (strlen(buffer_request) < 40) {
    memset(buffer_request + strlen(buffer_request), '\0', 40 - strlen(buffer_request));
  }

  strncpy(buffer_response, path_response, sizeof(buffer_response) - 1);
  buffer_response[sizeof(buffer_response) - 1] = '\0';  // Ensure null termination
  if (strlen(buffer_response) < 40) {
    memset(buffer_response + strlen(buffer_response), '\0', 40 - strlen(buffer_response) + 1);
  }

  memcpy(buffer1 + 2, buffer_request, sizeof(buffer_request) - 1);
  memcpy(buffer1 + 42, buffer_response, sizeof(buffer_response) - 1);
  buffer1[82] = '\0'; // Ensure null termination

  printf("buffer1: %s\n", buffer1);
  for (int i = 0; i < 84; ++i) {
    printf("%c", buffer1[i]);
  }
  //Send the message to the server
  if (write(fd_server_resquest, buffer1, 82) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }

  //Close the server pipe
  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }

  free(path_request);
  free(path_response);

  //Read the response from the server
  /*int fd_server_response = open(req_pipe_path, O_RDONLY);
  if (fd_server_response == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }*/

  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}
