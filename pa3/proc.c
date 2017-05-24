#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "proc.h"
#include "logger.h"

#define SQR(x) x*x

int close_fd ( local_id id, int fd ) {
    if ( close(fd) == 0 ) {
        log_closed_fd(id, fd);
        return 0;
    }

    return -1;
}

int create_pipe ( local_id id, int* fd ) {
    if ( pipe2(fd, O_NONBLOCK) == 0 ) {
        log_created_pipe(id, fd);
        return 0;
    }

    return -1;
}

// Creating pipes
void create_all_pipes ( proc_t* proc ) {
    for ( local_id i = 0; i < proc->process_count; i++ ) {
        for ( local_id j = 0; j < proc->process_count; j++ ) {
            if ( j!=i ) {
                int fd[2];
                create_pipe(proc->id, fd);
                proc->fd_read[j][i] = fd[0];
                proc->fd_writ[i][j] = fd[1];
            }
        }
    }
}

// Set process status to PARENT
void init_parent ( proc_t* proc, int process_count ) {
    proc->id = PARENT_ID;
    proc->process_count = process_count;
}

// Init file descriptors
void alloc_pipes ( proc_t* proc ) {
        
    int process_count = proc->process_count;

    proc->fd_read = (int **)malloc( sizeof(int *) * process_count );
    proc->fd_writ = (int **)malloc( sizeof(int *) * process_count );
    proc->fd_read[0] = malloc( sizeof(int) * SQR(process_count) );
    proc->fd_writ[0] = malloc( sizeof(int) * SQR(process_count) );
    for ( int i = 0; i < process_count; i++ ) {
        proc->fd_read[i] = ( *(proc->fd_read) + process_count * i );
        proc->fd_writ[i] = ( *(proc->fd_writ) + process_count * i );
    }
}

// Close unused fd of the process
void close_unused_pipes ( proc_t* proc ) {
    for ( local_id i = 0; i < proc->process_count; i++ ) {
        for ( local_id j = 0; j < proc->process_count; j++ ) {
            if ( i != proc->id && i != j ) {
                close_fd( proc->id, proc->fd_read[i][j] );
                close_fd( proc->id, proc->fd_writ[i][j] );
            }
        }
    }
}

/*
 * Spawn child processes. Here fork() happens.
 *
 * It is better to keep balance separate,
 * but because of nature of task and small
 * values we keep it in proc_t struct
 */
void spawn_procs ( proc_t* proc, balance_t* balance ) {
    for ( local_id i = 0; i < proc->process_count; i++ ) {
        proc->b_state.s_time = 0;
        if ( i != PARENT_ID && proc->id == PARENT_ID ) {
            pid_t pid = fork();
            if ( pid == 0 ) {
                proc->id = i;
                proc->b_state.s_balance = balance[i - 1];
                proc->b_state.s_balance_pending_in = 0;

                proc->b_history.s_history[0] = proc->b_state;
                proc->b_history.s_history_len = 1;
                proc->b_history.s_id = proc->id;

                log_started( proc, getpid(), getppid() );
            }
        }
    }
}

// Create pipes and child processes
void start_procs ( proc_t* proc, int process_count, balance_t* balance ) {

    // Init parent
    init_parent ( proc, process_count );

    // Alloc memory for pipes
    alloc_pipes ( proc );
    
    // Creating pipes
    create_all_pipes ( proc );

    // Spawn child processes. Here fork() happens.
    spawn_procs ( proc, balance );

    // Close unused fd
    close_unused_pipes ( proc );

}

