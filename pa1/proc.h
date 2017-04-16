#ifndef _PROC_H_
#define _PROC_H_

#include "ipc.h"
#include <sys/types.h>

typedef struct {
    local_id  id;
    uint8_t    process_count;
    int       *fd_read;
    int       *fd_writ;
} proc_t;

local_id spawn_procs ( proc_t* parent_proc, int process_count );

int close_pipe ( local_id id, int fd );

int create_pipe ( local_id id, int* fd );

void close_all_pipes ( proc_t* self );
#endif // _PROC_H_
