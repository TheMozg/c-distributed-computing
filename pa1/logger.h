#ifndef _LOGGER_H_
#define _LOGGER_H_

#define OUTPUR  STDOUT_FILENO
#define ERROR   STDERR_FILENO

#include "ipc.h"

int log_error ( const char *str );

int log_output ( const char *format, const char *str );

int log_started ( local_id id, pid_t pid, pid_t parent );

int log_done ( local_id id );

int log_received_all_started ( local_id id );

int log_received_all_done ( local_id id );

#endif //_LOGER_H_
