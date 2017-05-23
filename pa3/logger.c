#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "logger.h"
#include "pa2345.h"
#include "common.h"

#define LOG_START_ERROR "Cannot start log"
#define LOG_CLOSE_ERROR "Cannot close log"

#define EVENT_FILE      events_log     
#define PIPES_FILE      pipes_log     

int fd_event;
int fd_pipes;

int start_log () {
    fd_event = open( EVENT_FILE, O_TRUNC | O_CREAT | O_WRONLY, S_IRWXU | S_IRGRP | S_IROTH );
    fd_pipes = open( PIPES_FILE, O_TRUNC | O_CREAT | O_WRONLY, S_IRWXU | S_IRGRP | S_IROTH );

    if ( fd_event == -1 || fd_pipes == -1 ) {
        log_error(fd_event, LOG_START_ERROR);
        log_error(fd_pipes, LOG_START_ERROR);
        return -1;
    }
    
    return 0;
}

void close_log () {
    close(fd_event);
    close(fd_pipes);
}

void log_error ( int fd, const char *str ) {
    write( STDERR_FILENO, str, strlen(str) );
    write( STDERR_FILENO, "\n", 1 );

    write( fd, str, strlen(str) );
}

char* log_output ( int fd, const char *format, ... ) {
    int bufsize = sysconf(_SC_PAGESIZE);

    va_list args;
    va_start(args, format);

    char *buffer = (char *) malloc(bufsize);

    vsprintf(buffer, format, args);

    if ( fd == fd_event )
      write( STDOUT_FILENO, buffer, strlen(buffer) );
    write( fd, buffer, strlen(buffer) );

    va_end(args);

    return buffer;
}

char* log_started ( proc_t* proc, pid_t pid, pid_t parent ) {
    return log_output( fd_event, log_started_fmt, proc->b_state.s_time, proc->id,
            pid, parent, proc->b_state.s_balance );
}

char* log_done ( proc_t* proc ) {
    return log_output( fd_event, log_done_fmt, proc->b_state.s_time, proc->id, proc->b_state.s_balance );
}

void log_received_all_started ( proc_t* proc ) {
    log_output( fd_event, log_received_all_started_fmt, proc->b_state.s_time, proc->id );
}

void log_received_all_done ( proc_t* proc ) {
    log_output( fd_event, log_received_all_done_fmt, proc->b_state.s_time, proc->id );
}

void log_created_pipe ( local_id id, int* fd ) {
    log_output( fd_pipes, log_created_pipe_fmt, id, fd[0], fd[1] );
}

void log_closed_fd ( local_id id, int fd ) {
    log_output( fd_pipes, log_closed_fd_fmt, id, fd );
}

void log_transfer_out ( proc_t* proc, TransferOrder* trans ) {
    log_output( fd_event, log_transfer_out_fmt, proc->b_state.s_time,
            trans->s_src, trans->s_amount, trans->s_dst );
}

void log_transfer_in ( proc_t* proc, TransferOrder* trans ) {
    log_output( fd_event, log_transfer_in_fmt, proc->b_state.s_time,
            trans->s_dst, trans->s_amount, trans->s_src );
}
