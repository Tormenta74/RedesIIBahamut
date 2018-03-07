#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "finder.h"

int tout_seconds;

int main(int argc, char *argv[]) {
    int status, flaggy_boy;
    char *ctype;
    void *out;

    if (argc != 4) {
        printf("Uso: ./test resource input timeout\n");
        exit(1);
    }

    status = finder_setup();
    if (status == ERR) {
        printf("finder_setup failed\n");
        exit(1);
    }

    tout_seconds = atoi(argv[3]);

    status = finder_load(argv[1], argv[2], strlen(argv[2])+1, &out, &ctype, &flaggy_boy);
    if (status == ERR) {
        printf("finder_load failed\n");
        exit(1);
    }

    printf("%s\n", (char*)out);

    finder_clean();
    free(out);

    return 0;
}
