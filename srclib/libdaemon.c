#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "globals.h"
#include "libdaemon.h"

int daemonize(const char *proc_name, const char *root_path) {
    pid_t pid = fork();

    if(pid > 0) { exit(OK); }
    if(pid < 0) { exit(ERR); }

    umask(0);
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(proc_name, LOG_CONS|LOG_PID|LOG_NDELAY, LOG_LOCAL3);
    print("Sending server to background.");

    if(setsid() < 0) {
        print("Error creating session for the server.");
        return ERR;
    }

    if((chdir(root_path)) < 0) {
        print("Error changing directory to \"%s\".", root_path);
        return ERR;
    }

    print("Closing all file descriptors.");
    int i, ts=getdtablesize();
    for(i=0; i<ts; i++) {
        close(i);
    }

    return OK;
}
