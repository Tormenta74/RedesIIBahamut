#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "cgi.h"
#include "remap-pipe-fds.h"

#define R 0
#define W 1

#define	PARENT_READ     readpipe[R]
#define	CHILD_WRITE     readpipe[W]
#define CHILD_READ      writepipe[R]
#define PARENT_WRITE	writepipe[W]

int fork_exec(const char *program, const char *resource, const char *input, int len, char **output) {
    int status = ERR, nread;
    int writepipe[2], readpipe[2];
    pid_t pid;
    char buffer[MAX_SCRIPT_LINE_OUTPUT];

    //
    // open them pipes
    //

    status = pipe(writepipe);
    if(status) {
        printf("pipe failed yo\n");
        return ERR;
    }

    status = pipe(readpipe);
    if(status) {
        printf("pipe failed yo\n");
        return ERR;
    }

    //
    // fork this shit
    //

    pid = fork();

    //
    // am child, yo
    //

    if(pid == 0) {

        // close them not wanted pipes

        close(PARENT_WRITE);
        close(PARENT_READ);

        remap_pipe_stdin_stdout(CHILD_READ, CHILD_WRITE);

        execlp(program, program, resource, (char*)NULL);

        // if we got here, we fucked up
        
        return ERR;

    } else {
        //
        // am parent here aight
        //

        // close them not wanted pipes

        close(CHILD_READ);
        close(CHILD_WRITE);

        // give my son some time

        //sleep(1000);

        // tell him what it's all about

        write(PARENT_WRITE, input, len);
        close(PARENT_WRITE);

        // hear what it has to say

        nread = read(PARENT_READ, buffer, MAX_SCRIPT_LINE_OUTPUT);

        if(nread <= 0) {
            return ERR;
        }

        *output = strndup(buffer, nread);

        return OK;
    }
}

regex_t py, php;

int cgi_module_setup() {
    int status;

    status = regcomp(&py, "^.*\\.py$", 0);
    if(status) {
        print("Could not compile the Python extension regex.");
        return ERR;
    }

    status = regcomp(&php, "^.*\\.php$", 0);
    if(status) {
        print("Could not compile the PHP extension regex.");
        regfree(&py);
        return ERR;
    }

    return OK;
}

void cgi_module_clean() {
    regfree(&py);
    regfree(&php);
}

int cgi_exec_script(const char *resource, const char *input, int inlen, char **output) {
    int status;
    char errbuf[128];

    if(!resource || !input) {
        return ERR;
    }

    status = regexec(&py, resource, 0, NULL, 0);
    if(!status) {
        // match! launch python interpreter
        fork_exec("python", resource, input, inlen, output);
        return OK;
    } else if(status != REG_NOMATCH) {
        regerror(status, &py, errbuf, 128);
        print("Regex (.py extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&php, resource, 0, NULL, 0);
    if(!status) {
        // match! launch php interpreter
        fork_exec("php", resource, input, inlen, output);
        return OK;
    } else if(status != REG_NOMATCH) {
        regerror(status, &php, errbuf, 128);
        print("Regex (.php extension) match failed: %s", errbuf);
        return ERR;
    }

    // no match at all
    print("Resource does not match any of the supported scripts.");
    return NO_MATCH;
}

