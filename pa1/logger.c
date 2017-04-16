#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>

#include "logger.h"
#include "pa1.h"

void log_error ( const char *str ) {
    write( STDERR_FILENO, str, sizeof(str) / sizeof(char) );
    write( STDERR_FILENO, "\n", 1 );
}

void log_output ( const char *format, ... ) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void log_started ( local_id id, pid_t pid, pid_t parent ) {
    log_output( log_started_fmt, id, pid, parent );
}

void log_done ( local_id id ) {
    log_output( log_done_fmt, id );
}

void log_received_all_started ( local_id id ) {
    log_output( log_received_all_started_fmt, id );
}

void log_received_all_done ( local_id id ) {
    log_output( log_received_all_done_fmt, id );
}

void log_open_pipe ( int fd ) {

}
