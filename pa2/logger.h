#ifndef _LOGGER_H_
#define _LOGGER_H_

#define OUTPUR  STDOUT_FILENO
#define ERROR   STDERR_FILENO

#include "ipc.h"
#include "proc.h"

static const char * const log_created_pipe_fmt =
    "Process %d CREATED pipe (read fd %d, write fd %d)\n";

static const char * const log_closed_fd_fmt =
    "Process %d CLOSED fd %d\n";


extern int fd_event;
extern int fd_pipes;

int start_log ();

void close_log ();

void log_error ( int fd, const char *str );

char* log_output ( int fd, const char *format, ... );

char* log_started ( proc_t*, pid_t pid, pid_t parent );

char* log_done ( proc_t* proc );

void log_received_all_started ( proc_t* proc );

void log_received_all_done ( proc_t* proc );

void log_created_pipe ( local_id id, int* fd );

void log_closed_fd ( local_id id, int fd );
#endif //_LOGER_H_
