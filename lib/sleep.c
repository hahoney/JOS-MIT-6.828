#include <inc/lib.h>

// Waits until exits.
void
sleep(int interval)
{ 
    int end = interval + sys_time_msec();
    while (sys_time_msec() < end) {}
}
