#include "api.h"

#include <fcntl.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <sys/stat.h>
#include <sys/types.h>

int id_session = -1 ;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  //Create the paths
  size_t len_request =  strlen(req_pipe_path) + 1;
  char *path_request = malloc(len_request);
  snprintf(path_request, len_request, "%s", req_pipe_path);

  size_t len_response = strlen(resp_pipe_path) + 1;
  char *path_response = malloc(len_response);
  snprintf(path_response, len_response, "%s", resp_pipe_path);
 
  //Create the pipes
  if(mkfifo(path_request, 0777) == -1){
    fprintf(stderr, "Error creating request pipe\n");
    return 1;
  }

  if(mkfifo(path_response, 0777) == -1){
    fprintf(stderr, "Error creating response pipe\n");
    return 1;
  }

  //Connect to the server pipe
  int fd_server = open(server_pipe_path, O_WRONLY);
  if (fd_server == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }

  //Set the message to send to the server
  char buffer[82] ;

  buffer[0] = '1';
  buffer[1] = '\0';

  if (len_request < 40){
    strcat(buffer, path_request);
    memset(buffer + 2 , '\0', 40 ); 
  }
  if (len_response < 40){
    strcat(buffer, path_response);
    memset(buffer + 42 , '\0', 40 ); 
  }

  strcat(buffer, path_request);
  strcat(buffer, path_response);

  //Send the message to the server

  if (write(fd_server, buffer, 82) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }

  //Close the server pipe
  if (close(fd_server) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }

  free(path_request);
  free(path_response);
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
