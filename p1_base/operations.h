struct ThreadArgs {
    int fd_input, fd_output;
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    sem_t *thread_semaphore;
    struct Pthread *pthread_list;
};

struct Pthread{
    int id ;
    pthread_t *thread;   
};

/// Redirects the standard input and output to the given file descriptors or vice versa.
/// @param fd_input File descriptor to read the commands from.
/// @param fd_output File descriptor to write the output to.
/// @param saved_stdin Standard input file descriptor.
/// @param saved_stdout Standard output file descriptor.
/// @param flag Determines whether to redirect the standard input and output to the given file descriptors or vice versa.
void redirectStdinStdout(int fd_input, int fd_output, int saved_stdin, int saved_stdout, char *flag);

void remove_Pthread_list(struct Pthread *Pthread_list, unsigned int  max_threads , pthread_t *thread);

pthread_t *get_Pthread(struct Pthread *Pthread_list, unsigned int max_threads , int *id);

unsigned int get_free_Pthread_index(struct Pthread *Pthread_list, unsigned int max_threads);

void set_List_Pthreads(struct Pthread *Pthread_list, unsigned int max_threads);