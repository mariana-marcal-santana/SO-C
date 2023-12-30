#include "api.h"
#include "common/io.h"

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

  int buffer_to_server[1] ;
  buffer_to_server[0] = 2;
  // Make request to server
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  printf("buffer to server %d\n", buffer_to_server[0]);
  if (write(fd_server_resquest, buffer_to_server, sizeof(buffer_to_server)) == -1) {
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
  
  int buffer_to_server[4];
  buffer_to_server[0] = 3;
  buffer_to_server[1] = (int) event_id;
  buffer_to_server[2] = (int) num_rows;
  buffer_to_server[3] = (int) num_cols;

  for (int i = 0; i < 4; i++) {
    printf("buffer to server %d\n", buffer_to_server[i]);
  }
  
  // Open request pipe
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Send request
  if (write(fd_server_resquest, buffer_to_server, 4) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }
  // Close request pipe
  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  // Open response pipe
  int fd_server_response = open(path_fifo_response, O_RDONLY);
  if (fd_server_response == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Read response
  int buffer_from_server[1];
  if (read(fd_server_response, buffer_from_server, 1) == -1) {
    fprintf(stderr, "Error reading from server pipe\n");
    return 1;
  }
  // Close response pipe
  if (close(fd_server_response) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  // Convert response to int an return
  return buffer_from_server[0];
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  int buffer_to_server[num_seats * 2 + 3];
  buffer_to_server[0] = 4;
  buffer_to_server[1] = (int) event_id;
  buffer_to_server[2] = (int) num_seats;
  size_t count = 3;
  for (size_t i = 0; i < num_seats; i++) {
    buffer_to_server[count] = (int) xs[i];
    count++;
  }
  buffer_to_server[count] = -1 ;
  count++;
  for (size_t i = 0; i < num_seats; i++) {
    buffer_to_server[count] = (int) ys[i];
    count++;
  }
  
  // Open request pipe
  int fd_server_resquest = open(path_fifo_request, O_WRONLY);
  if (fd_server_resquest == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Send request
  if (write(fd_server_resquest, buffer_to_server, 527) == -1) {
    fprintf(stderr, "Error writing to server pipe\n");
    return 1;
  }
  // Close request pipe
  if (close(fd_server_resquest) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  // Open response pipe
  int fd_server_response = open(path_fifo_response, O_RDONLY);
  if (fd_server_response == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  // Read response
  int buffer_from_server[1];
  if (read(fd_server_response, buffer_from_server, sizeof(buffer_from_server)) == -1) {
    fprintf(stderr, "Error reading from server pipe\n");
    return 1;
  }
  // Close response pipe
  if (close(fd_server_response) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  return buffer_from_server[0];
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  int buffer_to_server[2];
  buffer_to_server[0] = 5;
  buffer_to_server[1] = (int) event_id;

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
  // Read first response
  int first_buffer_from_server[3];
  int fd_server_response = open(path_fifo_response, O_RDONLY);
  if (fd_server_response == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  if (read(fd_server_response, first_buffer_from_server, 3) == -1) {
    fprintf(stderr, "Error reading from server pipe\n");
    return 1;
  }
  // Close response pipe
  if (close(fd_server_response) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  // Check if response is valid
  if (first_buffer_from_server[0] == 1) {
    return 1;
  }
  // Read second response
  int num_rows = first_buffer_from_server[1];
  int num_cols = first_buffer_from_server[2];
  int num_seats = num_rows * num_cols;
  
  int second_buffer_from_server[num_seats + 1];
  fd_server_response = open(path_fifo_response, O_RDONLY);
  if (fd_server_response == -1) {
    fprintf(stderr, "Error opening server pipe\n");
    return 1;
  }
  if (read(fd_server_response, second_buffer_from_server, sizeof(second_buffer_from_server)) == -1) {
    fprintf(stderr, "Error reading from server pipe\n");
    return 1;
  }
  // Close response pipe
  if (close(fd_server_response) == -1) {
    fprintf(stderr, "Error closing server pipe\n");
    return 1;
  }
  // Check if response is valid
  if (second_buffer_from_server[0] == 1) {
    return 1;
  }
  // Write response to out_fd
  int i = 1;
  while (i <= num_seats) {
    write(out_fd, second_buffer_from_server + i, sizeof(int));
    if (i % num_cols == 0) {
      write(out_fd, "\n", 1);
    }
    else {
      write(out_fd, " ", 1);
    }
    i++;
  }
  return 0 ;
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
