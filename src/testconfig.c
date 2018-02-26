#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "globals.h"
#include "config.h"

int main(int argc, char *argv[]) {
    int status;
    struct server_options so;

    status = config_parse("server.conf", &so);
    if(status == ERR) {
        printf("Parsing failed.\n");
        exit(ERR);
    }

    config_print(&so);

    return OK;
}
