#include "api.h"

#include <fcntl.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int id_session = -1 ;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  //Create the paths
  size_t len_request = strlen("../tmp") + strlen(req_pipe_path) + 1;
  char *path_request = malloc(len_request);
  snprintf(path_request, len_request, "../tmp%s", req_pipe_path);

  size_t len_response = strlen("../tmp") + strlen(resp_pipe_path) + 1;
  char *path_response = malloc(len_response);
  snprintf(path_response, len_response, "../tmp%s", resp_pipe_path);

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
