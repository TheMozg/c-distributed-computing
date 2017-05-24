#include "lamport.h"

timestamp_t l_time = 0;

timestamp_t get_lamport_time( ) {
    return l_time;
}

timestamp_t l_time_cmp_set ( timestamp_t c_time ) {
    return l_time = l_time > c_time ? l_time : c_time;
}

timestamp_t get_inc_l_time( ) {
    return ++l_time;
}
