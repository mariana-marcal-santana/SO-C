#ifndef EMS_OPERATIONS_H
#define EMS_OPERATIONS_H

#include <stddef.h>



/// Initializes the EMS state.
/// @param delay_ms State access delay in milliseconds.
/// @return 0 if the EMS state was initialized successfully, 1 otherwise.
int ems_init(unsigned int delay_ms);

/// Destroys the EMS state.
int ems_terminate();

/// Creates a new event with the given id and dimensions.
/// @param event_id Id of the event to be created.
/// @param num_rows Number of rows of the event to be created.
/// @param num_cols Number of columns of the event to be created.
/// @return 0 if the event was created successfully, 1 otherwise.
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols);

/// Creates a new reservation for the given event.
/// @param event_id Id of the event to create a reservation for.
/// @param num_seats Number of seats to reserve.
/// @param xs Array of rows of the seats to reserve.
/// @param ys Array of columns of the seats to reserve.
/// @return 0 if the reservation was created successfully, 1 otherwise.
int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys);

/// Prints the given event.
/// @param event_id Id of the event to print.
/// @param fd_output File descriptor to write the output to.
/// @return 0 if the event was printed successfully, 1 otherwise.
int ems_show(unsigned int event_id, int fd_output);

/// Prints all the events.
/// @return 0 if the events were printed successfully, 1 otherwise.
int ems_list_events();

/// Waits for a given amount of time.
/// @param delay_us Delay in milliseconds.
void ems_wait(unsigned int delay_ms);

/// Processes the commands from the given file descriptor.
/// @param fd_input File descriptor to read the commands from.
/// @param fd_output File descriptor to write the output to.
void ems_process(int fd_input, int fd_output);

/// Processes the commands from the given file descriptor using threads.
/// @param fd_input File descriptor to read the commands from.
/// @param fd_output File descriptor to write the output to.
/// @param max_threads Maximum number of threads to use.
void ems_process_with_threads(int fd_input, int fd_output, unsigned int max_threads);

/// Processes each command from the given file descriptor in a separate thread.
/// @param args (struct ThreadArgs *) Arguments for the thread.
/// @return 
void *ems_process_thread(void * args);

#endif  // EMS_OPERATIONS_H
