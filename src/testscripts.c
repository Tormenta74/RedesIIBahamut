#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "finder.h"
#include "cgi.h"

int main(int argc, char *argv[]) {
    int inlen, flag;
    long status;
    char *out, *ctype;

    if(argc != 3) {
        printf("Uso: ./test resource input\n");
        exit(ERR);
    }

    inlen = strlen(argv[2]);

    if(finder_setup() == ERR) {
        printf("Failed to setup finder module.\n");
        return ERR;
    }

    status = finder_load(argv[1], argv[2], inlen, &out, &ctype, &flag);
    if(status == ERR) {
        printf("Failed to load requested file.\n");
        return ERR;
    }

    if(flag == NO_MATCH) {
        printf("Script \"%s\" with input \"%s\" returned:\n(no ContentType) %s\n",
                argv[1], argv[2], out);
    } else {
        printf("Script \"%s\" with input \"%s\" returned:\n(%s) %s\n",
                argv[1], argv[2], ctype, out);
    }

    finder_clean();

    return OK;
}
