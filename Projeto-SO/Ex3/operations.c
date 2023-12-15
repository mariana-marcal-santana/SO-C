#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "constants.h"
#include "ems_operations.h"
#include "operations.h"

void redirectStdinStdout(int fd_input, int fd_output, int saved_stdin, int saved_stdout, char *flag) {
    if (strcmp(flag, "FD") == 0) {
        if (dup2(fd_output, STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout");
            close(fd_output);
            return;
        }
        if (dup2(fd_input, STDIN_FILENO) == -1) {
            perror("Failed to redirect stdin");
            close(fd_input);
            return;
        }
    } 
    else if (strcmp(flag, "STD") == 0) {
        if (dup2(saved_stdin, STDIN_FILENO) == -1) {
            perror("Failed to restore stdin");
            close(fd_input);
            return;
        }
        if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
            perror("Failed to restore stdout");
            close(fd_output);
            return;
        }
    }
}

void set_List_Pthreads(struct Pthread *Pthread_list, unsigned int max_threads) {
    for (unsigned int i = 0; i < max_threads; i++) {
        Pthread_list[i].id = -1;
        Pthread_list[i].thread = malloc(sizeof(pthread_t));
    }
}

void free_list_Pthreads(struct Pthread *Pthread_list, unsigned int max_threads) {
    for (unsigned int i = 0; i < max_threads; i++) {
        free(Pthread_list[i].thread);
    }
}

unsigned int get_index_thread(struct Pthread *Pthread_list, unsigned int max_threads, unsigned int *id) {
    int id_thread = (int)*id;
    for (unsigned int i = 0; i < max_threads; i++) {
        if (Pthread_list[i].id == id_thread ) {
            return i;
        }
    }
    return 0;
}
