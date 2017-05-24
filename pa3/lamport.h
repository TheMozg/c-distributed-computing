#ifndef _LAMPORT_TIME_H_
#define _LAMPORT_TIME_H_

#include "ipc.h"
#include "banking.h"

timestamp_t l_time;

timestamp_t get_lamport_time( );

timestamp_t l_time_cmp_set ( timestamp_t c_time );

timestamp_t get_inc_l_time( );

#endif // _LAMPORT_TIME_H_
