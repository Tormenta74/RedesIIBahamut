#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "globals.h"
#include "config.h"

int config_parse(char* filename, struct server_options *so) {
    size_t len;
    ssize_t read;
    char *line = NULL;
    char option[17], rest[128 - 17];
    FILE *f;
    if(!filename || !so) {
        return ERR;
    }

    if(!(f = fopen(filename, "r"))) {
        return ERR;
    }

    while((read = getline(&line, &len, f)) != ERR) {
        // line is a comment
        if(line[0] == '#') {
            continue;
        }

        if(sscanf(line, "%s = %s", option, rest) != 2) {
            // bad formatting
            return ERR;
        }

        // I wish C had strings, and they were good enough for switches
        if(strcmp(option, "server_root")) {
            if(!(so->server_root = strdup(rest))) {
                // failed to get memory
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "server_signature")) {
            if(!(so->server_signature = strdup(rest))) {
                // failed to get memory
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "max_clients")) {
            if((so->max_clients = atoi(rest)) <= 0) {
                // this shit ain't right
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "listen_port")) {
            if((so->listen_port = (uint16_t)atol(rest)) <= 0) {
                // this shit ain't right
                return ERR;
            }
            continue;
        }
    } /* while */

    if(line) {
        free(line);
    }

    return OK;
}

// if "${workspace}/httprc" file exists, it will try to load it
// else, we load some default values
int config_load_defaults(struct server_options *so);
