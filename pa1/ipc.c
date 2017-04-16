#include "ipc.h"
#include "proc.h"
#include <unistd.h>

int send(void * self, local_id dst, const Message * msg) {
    proc_t * proc = self;
    write(proc->fd_writ[dst], msg, sizeof(Message));
    return 0;
}

int send_multicast(void * self, const Message * msg) {
    return -1;
}

int receive(void * self, local_id from, Message * msg) {
    return -1;
}

int receive_any(void * self, Message * msg) {
    proc_t * proc = self;
    read(proc->fd_read[proc->id], msg, sizeof(Message));
    return 0;
}
