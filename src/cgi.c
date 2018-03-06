#include <fcntl.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "cgi.h"
#include "remap-pipe-fds.h"

#define R 0
#define W 1

#define PARENT_READ     readpipe[R]
#define CHILD_WRITE     readpipe[W]
#define CHILD_READ      writepipe[R]
#define PARENT_WRITE    writepipe[W]

/*
 * Description: Executes in a child process the "program" interpreter with the "resource" script,
 *              providing "input" of length "inlen" through the child's redirected stdi/o. It's
 *              the responsability of the caller to free the memory reserved for "output".
 *
 * In:
 * const char *program: name of the interpreter (shell name, e.g. "python", not "/usr/bin/python")
 * const char *resource: path to the desired script to be interpreted
 * const char *input: the string which the script is to receive via its standard input
 * int inlen: length of the input
 * char **output: pointer to the memory zone where to write the output of the script
 * int wait_s: desired timeout in seconds
 *
 * Return: ERR in case of failure at any point. Size of the output string otherwise.
 */
long cgi_exec_script(const char *program, const char *resource, const char *input, int inlen, char **output, int wait_s) {
    int status = ERR, nread = 0;
    long output_size = 0;
    int writepipe[2], readpipe[2];
    pid_t pid;
    char buffer[MAX_SCRIPT_LINE_OUTPUT], aux[MAX_SCRIPT_LINE_OUTPUT];

    //
    // open them pipes
    //

    status = pipe(writepipe);
    if (status) {
        print("pipe failed.");
        return ERR;
    }

    status = pipe(readpipe);
    if (status) {
        print("pipe failed.");
        return ERR;
    }

    //
    // fork this
    //

    pid = fork();

    if (pid == 0) {

        //
        // am child, yo
        //

        // close them not wanted pipes

        close(PARENT_WRITE);
        close(PARENT_READ);

        remap_pipe_stdin_stdout(CHILD_READ, CHILD_WRITE);

        execlp(program, program, resource, (char*)NULL);

        // if we got here, we f***** up

        exit(ERR);

    } else {

        //
        // am parent here aight
        //

        // close them not wanted pipes

        close(CHILD_READ);
        close(CHILD_WRITE);

        // tell him what it's all about

        write(PARENT_WRITE, input, inlen);
        close(PARENT_WRITE);

        // prepare to hear what it has to say

        fd_set microset;
        struct timeval timeout;
        int sret;

        // a whole fd_set for my child

        FD_ZERO(&microset);
        FD_SET(PARENT_READ, &microset);

        // but I tell ya, I'm busy!

        timeout.tv_sec = wait_s;
        timeout.tv_usec = 0;

        bzero(aux, MAX_SCRIPT_LINE_OUTPUT);

        // now let's hear it

        sret = select(PARENT_READ + 1, &microset, NULL, NULL, &timeout);
        //sret = select(PARENT_READ + 1, &microset, NULL, NULL, NULL);
        if(sret == -1) {

            // oh boy
            print("cgi: Error selecting on the child stdout.");
            return ERR;

        } else if(sret == 0) {

            // too late, son
            print("cgi: Script timed out.");

            // now I have to end you
            kill(pid, SIGTERM);

            return TIMEOUT;

        } else {
            //nread = read(PARENT_READ, buffer, MAX_SCRIPT_LINE_OUTPUT);
            //if(nread <= 0) {
            //    print("cgi: Select triggered, but read gave nothing.");
            //    return ERR;
            //}
            //sprintf(aux, "%s%s", aux, buffer);
            //output_size += nread;

            while ((nread = read(PARENT_READ, buffer, MAX_SCRIPT_LINE_OUTPUT)) >= 0) {
                sprintf(aux, "%s%s", aux, buffer);
                output_size += nread;

                // "\n" means there was a new line (in python)
                // "\000" means there was a new line (in php)
                //
                // Sigh.
                //
                // "\r\n" afterwards means the script sent a line with just that
                if (strstr(buffer, "\n\r\n") != NULL
                        || strstr(buffer, "\000\r\n") != NULL) {
                    break;
                }
            }
        }

        *output = strndup(aux, output_size);

        return output_size;
    }
}

