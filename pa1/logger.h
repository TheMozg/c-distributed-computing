#ifndef _LOGGER_H_
#define _LOGGER_H_

#define OUTPUR  STDOUT_FILENO
#define ERROR   STDERR_FILENO

#include "ipc.h"

void log_error ( const char *str );

void log_output ( const char *format, ... );

void log_started ( local_id id, pid_t pid, pid_t parent );

void log_done ( local_id id );

void log_received_all_started ( local_id id );

void log_received_all_done ( local_id id );

#endif //_LOGER_H_
