#include <stdio.h> // Won't be needed after logger
#include <stdlib.h>
#include <unistd.h>
#include "proc.h"
#include "ipc.h"
#include "pa1.h" // Won't be needed after logger

local_id spawn_procs ( proc_t* proc, int process_count ) {
        
    // Init file descriptors
    proc->fd_read = malloc( sizeof(int) * process_count );
    proc->fd_writ = malloc( sizeof(int) * process_count );
    proc->id = PARENT_ID;

    for ( local_id i = 0; i < process_count; i++ ) {
        int fd[2];
        pipe(fd);
        proc->fd_read[i] = fd[0];
        proc->fd_writ[i] = fd[1];
    }

    // Spawn child processes
    for ( local_id i = 0; i < process_count; i++ ) {
        if ( i != PARENT_ID && proc->id == PARENT_ID ) {
            pid_t pid = fork();
            if ( pid == 0 ) {
                proc->id = i;
                // TODO: make logger here
                printf( log_started_fmt, proc->id, getpid(), getppid() );
            }
        }
    }

    // Close write fd of current process since it does not message itself
    close( proc->fd_writ[proc->id] );  

    // Close read fd of all other processes. Read messages only for this process
    for ( local_id i = 0; i < process_count; i++ ) {
        if ( i != proc->id ) {
            close(proc->fd_read[i]);
        }
    }

    return proc->id;
}
