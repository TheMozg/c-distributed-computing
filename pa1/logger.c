#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "logger.h"
#include "pa1.h"

int log_error ( const char *str ) {
    write( STDERR_FILENO, str, sizeof(str) / sizeof(char) );
    write( STDERR_FILENO, "\n", 1 );
    return -1;
}

int log_output ( const char *format, const char *str ) {
    printf( format, str );
    return 0;
}

int log_started ( local_id id, pid_t pid, pid_t parent ) {
    printf( log_started_fmt, id, pid, parent );
    return 0;
}

int log_done ( local_id id ) {
    printf( log_done_fmt, id );
    return 0;
}

int log_received_all_started ( local_id id ) {
    printf( log_received_all_started_fmt, id );
    return 0;
}

int log_received_all_done ( local_id id ) {
    printf( log_received_all_done_fmt, id );
    return 0;
}
