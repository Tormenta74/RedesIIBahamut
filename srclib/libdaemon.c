#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "globals.h"
#include "libdaemon.h"

/*
 * Description: Sends the process to the background, opens the log with the desired
 *              name, changes the working directory to the desired path and closes all
 *              file descriptors.
 *
 * In:
 * const char *proc_name: the name with which the server will log to the system
 * const char *root_path: path to the desired directory
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
int daemonize(const char *proc_name, const char *root_path) {
    pid_t pid = fork();

    // the parent code: exit inmediately
    if(pid > 0) {
        exit(OK);
    }
    // if fork returned a negative number, there is an error
    if(pid < 0) {
        exit(ERR);
    }

    // file creation permissions 0000
    umask(0);
    // set log priorities
    setlogmask(LOG_UPTO(LOG_INFO));
    // open the log
    openlog(proc_name, LOG_CONS|LOG_PID|LOG_NDELAY, LOG_LOCAL3);

    print("Sending server to background.");

    // create a new session for the process
    if(setsid() < 0) {
        print("Error creating session for the server.");
        return ERR;
    }

    // change current directory
    if((chdir(root_path)) < 0) {
        print("Error changing directory to \"%s\".", root_path);
        return ERR;
    }

    // close all file descriptors
    int i, ts = getdtablesize();
    for(i=0; i<ts; i++) {
        close(i);
    }

    return OK;
}
