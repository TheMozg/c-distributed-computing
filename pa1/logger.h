#ifndef _LOGGER_H_
#define _LOGGER_H_

#define OUTPUR  STDOUT_FILENO
#define ERROR   STDERR_FILENO

#include "ipc.h"

static const char * const log_created_pipe_fmt =
    "Process %d CREATED pipe %d\n";

static const char * const log_closed_pipe_fmt =
    "Process %d CLOSED pipe %d\n";


extern int fd_event;
extern int fd_pipes;

int start_log ();

void close_log ();

void log_error ( int fd, const char *str );

void log_output ( int fd, const char *format, ... );

void log_started ( local_id id, pid_t pid, pid_t parent );

void log_done ( local_id id );

void log_received_all_started ( local_id id );

void log_received_all_done ( local_id id );

void log_created_pipe ( local_id id, int fd );

void log_closed_pipe ( local_id id, int fd );
#endif //_LOGER_H_
