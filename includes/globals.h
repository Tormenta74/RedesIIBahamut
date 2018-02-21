#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <syslog.h>

#define OK  0
#define ERR -1

char log_buffer[1024];

//#ifdef VERBOSE
#define print(...) {\
    sprintf(log_buffer, __VA_ARGS__);\
    sprintf(log_buffer, "%s\n", log_buffer);\
    fprintf(stdout, log_buffer);\
}
//#else
//#define print(...) {\
//    sprintf(log_buffer, __VA_ARGS__);\
//    syslog(LOG_INFO, log_buffer);\
//}
//#endif /*VERBOSE*/

#endif /*_GLOBALS_H*/
