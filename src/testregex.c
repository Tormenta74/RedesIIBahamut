#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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

int main(int argc, char *argv[]) {

    int status = ERR, nread;
    char readbuffer[128];
    int writepipe[2], readpipe[2];
    pid_t pid;

    //
    // open them pipes
    //

    status = pipe(writepipe);
    if(status) {
        printf("pipe failed yo\n");
        exit(ERR);
    }

    status = pipe(readpipe);
    if(status) {
        printf("pipe failed yo\n");
        exit(ERR);
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

        /*
         * If I were a simple son, this is what I would do:

        // listen to pa

        nread = read(writepipe[R], readbuffer, 128);

        printf("child: Read %d bytes: \"%s\"\n", nread, readbuffer);

        // write my shit down

        status = write(readpipe[W], "General Kenobi!", 16);

        if(status <= 0) {
        exit(ERR);
        }

        exit(OK);

         * Sadly, I'm not.
         */

        remap_pipe_stdin_stdout(CHILD_READ, CHILD_WRITE);

        execlp("python", "python", "scripts/fahrenheit.py", (char*)NULL);
        //execlp("php", "php", "scripts/hi.php", (char*)NULL);
        //execlp("wc", "wc", (char*)NULL);
        //execlp("ls", "ls", (char*)NULL);

        // if we got here, we fucked up
        
        exit(ERR);

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

        write(PARENT_WRITE, "16.2", 5);
        close(PARENT_WRITE);

        // hear what it has to say

        nread = read(PARENT_READ, readbuffer, 128);

        printf("parent: Read %d bytes: \"%s\"\n", nread, readbuffer);

        return OK;
    }
}
