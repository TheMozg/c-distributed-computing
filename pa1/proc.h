#ifndef _PROC_H_
#define _PROC_H_

#include "ipc.h"
#include <sys/types.h>

typedef struct {
    local_id  id;
    local_id  process_count;
    int       **fd_read;
    int       **fd_writ;
} proc_t;

int close_fd ( local_id id, int fd );

int create_pipe ( local_id id, int* fd );

void create_all_pipes ( proc_t* proc );

void init_parent ( proc_t* proc, int process_count );

void alloc_pipes ( proc_t* proc );

void close_unused_pipes ( proc_t* proc );

void spawn_procs ( proc_t* proc );

local_id start_procs ( proc_t* proc, int process_count );

#endif // _PROC_H_
