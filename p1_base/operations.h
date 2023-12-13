struct ThreadArgs {
    int fd_input, fd_output, return_value;
    unsigned int event_id, delay, thread_id;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    struct Pthread *pthread_list;
    unsigned int max_threads;
    int current_thread_id;
};

struct Pthread{
    int id ;
    pthread_t *thread;   
    unsigned int wait ;
};

/// Redirects the standard input and output to the given file descriptors or vice versa.
/// @param fd_input File descriptor to read the commands from.
/// @param fd_output File descriptor to write the output to.
/// @param saved_stdin Standard input file descriptor.
/// @param saved_stdout Standard output file descriptor.
/// @param flag Determines whether to redirect the standard input and output to the given file descriptors or vice versa.
void redirectStdinStdout(int fd_input, int fd_output, int saved_stdin, int saved_stdout, char *flag);

void set_List_Pthreads(struct Pthread *Pthread_list, unsigned int max_threads);

void free_list_Pthreads(struct Pthread *Pthread_list, unsigned int max_threads);

unsigned int get_index_thread(struct Pthread *Pthread_list, unsigned int max_threads, unsigned int *index);