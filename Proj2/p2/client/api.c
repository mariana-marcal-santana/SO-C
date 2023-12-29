#include "api.h"
#include "operations.h"

#include <fcntl.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <sys/stat.h>
#include <sys/types.h>

char const* path_fifo_server;
char const* path_fifo_request;
char const* path_fifo_response;

int id_session;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  
  path_fifo_server = server_pipe_path;
  path_fifo_request = req_pipe_path;
  path_fifo_response = resp_pipe_path;

  //Create the paths
  size_t len_request =  strlen(req_pipe_path) + 1;
  char  *path_request = malloc(len_request);
  snprintf(path_request, len_request, "%s", req_pipe_path);

  size_t len_response = strlen(resp_pipe_path) + 1;
  char *path_response = malloc(len_response);
  snprintf(path_response, len_response, "%s", resp_pipe_path);
 
  //Connect to the server pipe
  int fd_server_resquest = open(path_fifo_server, O_WRONLY);
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
  char buffer_to_server[84], buffer_request[41], buffer_response[41];

  buffer_to_server[0] = '1';
  buffer_to_server[1] = '\0';

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

  memcpy(buffer_to_server + 2, buffer_request, sizeof(buffer_request) - 1);
  memcpy(buffer_to_server + 42, buffer_response, sizeof(buffer_response) - 1);

  //Send the message to the server
  if (write(fd_server_resquest, buffer_to_server, 82) == -1) {
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
  int fd_server_response = open(path_fifo_server, O_RDONLY);
  if (fd_server_response == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  
  char buffer_from_server[2];
  ssize_t bytes_read = read(fd_server_response, buffer_from_server, 2);
  if (bytes_read == -1) {
    fprintf(stderr, "Error reading from server pipe\n");
    return 1;
  }
 
  //Close the server pipe
  if (close(fd_server_response) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }

  //Set the session id
  id_session = atoi(buffer_from_server);
  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  char buffer_to_server[2];
  buffer_to_server[0] = '2';
  buffer_to_server[1] = '\0';
  
  // Make request to server
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  printf("buffer to server %s\n", buffer_to_server);
  if (write(fd_server_resquest, buffer_to_server, 2) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }

  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  char buffer_to_server[14]; // 2 op_code + 4 event_id + 4 num_rows + 4 num_cols
  buffer_to_server[0] = '3';
  buffer_to_server[1] = '\0';
  // Convert event_id to char vector an add to buffer
  char buffer_event_id[4];
  int_to_buffer(event_id, buffer_event_id);
  memcpy(buffer_to_server + 2, buffer_event_id, 4);
  // Typecast num_rows and num_cols to int
  unsigned int num_rows_int = (unsigned int)num_rows;
  unsigned int num_cols_int = (unsigned int)num_cols;
  // Convert num_rows and num_cols to char vector and add to buffer
  char buffer_num_rows[4];
  int_to_buffer(num_rows_int, buffer_num_rows);
  memcpy(buffer_to_server + 6, buffer_num_rows, 4);
  char buffer_num_cols[4];
  int_to_buffer(num_cols_int, buffer_num_cols);
  memcpy(buffer_to_server + 10, buffer_num_cols, 4);
  // Open request pipe
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Send request
  if (write(fd_server_resquest, buffer_to_server, 14) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }
  // Close request pipe
  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  char buffer_to_server[527]; // 2 op_code + 4 event_id + 6 num_seats + 257 xs + 257 ys
  buffer_to_server[0] = '4';
  buffer_to_server[1] = '\0';
  // Convert event_id to char vector an add to buffer
  char buffer_event_id[4];
  int_to_buffer(event_id, buffer_event_id);
  memcpy(buffer_to_server + 2, buffer_event_id, 4);
  // Typecast num_seats to int
  unsigned int num_seats_int = (unsigned int)num_seats;
  // Convert num_seats to char vector and add to buffer
  char buffer_num_seats[6];
  int_to_buffer(num_seats_int, buffer_num_seats);
  memcpy(buffer_to_server + 6, buffer_num_seats, 6);
  // Convert xs and ys to char vector and add to buffer ?????????
  return 0;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  char buffer_to_server[6]; // 2 op_code + 4 event_id
  buffer_to_server[0] = '5';
  buffer_to_server[1] = '\0';
  char buffer_event_id[4];
  int_to_buffer(event_id, buffer_event_id);
  memcpy(buffer_to_server + 2, buffer_event_id, 4);
  // Open request pipe
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Send request
  if (write(fd_server_resquest, buffer_to_server, 6) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }
  // Close request pipe
  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  return 0;
}

int ems_list_events(int out_fd) {
  char buffer_to_server[2];
  buffer_to_server[0] = '6';
  buffer_to_server[1] = '\0';
  // Open request pipe
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Send request
  if (write(fd_server_resquest, buffer_to_server, 2) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }
  // Close request pipe
  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  return 0;
}
