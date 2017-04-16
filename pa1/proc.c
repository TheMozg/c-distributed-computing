#include <stdio.h> // Won't be needed after logger
#include <stdlib.h>
#include <unistd.h>
#include "proc.h"
#include "ipc.h"
#include "pa1.h" // Won't be needed after logger
#include "logger.h"

int close_pipe ( local_id id, int fd ) {
    if ( close(fd) == 0 ) {
        log_closed_pipe(id, fd);
        return 0;
    }

    return -1;
}

int create_pipe ( local_id id, int* fd ) {
    if ( pipe(fd) == 0 ) {
        log_created_pipe(id, fd[0]);
        log_created_pipe(id, fd[1]);
        return 0;
    }

    return -1;
}

void close_all_pipes ( proc_t* self ) {
    for ( int i = 0; i < self->process_count; i++ ) {
        close(self->fd_read[i]);
        close(self->fd_writ[i]);
    }
}
// Create pipes and child processes
local_id spawn_procs ( proc_t* proc, int process_count ) {
        
    // Init file descriptors
    proc->fd_read = malloc( sizeof(int) * process_count );
    proc->fd_writ = malloc( sizeof(int) * process_count );
    proc->id = PARENT_ID;
    proc->process_count = process_count;

    for ( local_id i = 0; i < process_count; i++ ) {
        int fd[2];
        create_pipe(i, fd);
        proc->fd_read[i] = fd[0];
        proc->fd_writ[i] = fd[1];
    }

    // Spawn child processes
    for ( local_id i = 0; i < process_count; i++ ) {
        if ( i != PARENT_ID && proc->id == PARENT_ID ) {
            pid_t pid = fork();
            if ( pid == 0 ) {
                proc->id = i;
                log_started( proc->id, getpid(), getppid() );
            }
        }
    }

    // Close write fd of current process since it does not message itself
    close_pipe( proc->id, proc->fd_writ[proc->id] );  

    // Close read fd of all other processes. Read messages only for this process
    for ( local_id i = 0; i < process_count; i++ ) {
        if ( i != proc->id ) {
            close_pipe( proc->id, proc->fd_read[i] );
        }
    }

    return proc->id;
}

