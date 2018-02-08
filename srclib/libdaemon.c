#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "globals.h"
#include "libdaemon.h"

int daemonize(const char* proc_name) {
    pid_t pid = fork();

    if(pid > 0) exit(OK);
    if(pid < 0) exit(ERR);

    umask(0);
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(proc_name, LOG_CONS|LOG_PID|LOG_NDELAY,LOG_LOCAL3);
    syslog(LOG_ERR,"Sending server to background.");

    if(setsid() < 0) {
        syslog(LOG_ERR,"Error creating session for the server.");
        return ERR;
    }

    if((chdir("/")) < 0) {
        syslog(LOG_ERR,"Error changing directory to \"/\".");
        return ERR;
    }

    syslog(LOG_INFO, "Closing all file descriptors.");
    int i, ts=getdtablesize();
    for(i=0;i<ts;i++) 
        close(i);

    return OK;
}
