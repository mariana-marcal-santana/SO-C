# SO-C

This project was done by:

Mariana Santana - ist1106992

João Rodrigues - ist1106221

To run the project compile it (command make) and execute ./ems path_to_server_pipe on the server terminal followed by ./client path_to_request_pipe path_to_response_pipe path_to_server_fifo path_to_.jobs_file on one or more client terminals.

To test the signal interruption execute in any terminal kill -SIGUSR1 PID , where PID is in the first line printed on the server terminal upon execution.

To eliminate the executable files as well as the files resulting from the execution of the project use command make clean.
