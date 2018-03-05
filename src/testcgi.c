#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "cgi.h"
#include "remap-pipe-fds.h"

int main(int argc, char *argv[]) {
    int status;
    char *out;

    if (argc != 3) {
        printf("Uso: ./test resource input\n");
        exit(1);
    }

    status = cgi_module_setup();
    if (status == ERR) {
        printf("cgi_module_setup failed\n");
        exit(1);
    }

    status = cgi_exec_script(argv[1], argv[2], strlen(argv[2])+1, &out);
    if (status == ERR) {
        printf("cgi_exec_script failed\n");
        exit(1);
    }

    printf("%s\n", out);

    cgi_module_clean();
    free(out);

    return 0;
}
