#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "globals.h"
#include "config.h"

/*
 * Description: Opens and reads a configuration file with a format as follows. Note that empty lines
 *              are valid as long as they don't contain spaces, and there must be exactly one space
 *              before and after the "=".
 *
 * server.conf:
 *   # lines starting with # are comments
 *
 *   key = value
 *
 * In:
 * char *filename: path to the configuration file
 * const char *resource: path to the desired script to be interpreted
 * const char *input: the string which the script is to receive via its standard input
 * int inlen: length of the input
 * char **output: pointer to the memory zone where to write the output of the script
 *
 * Return: ERR in case of failure at any point. Size of the output string otherwise.
 * */
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

    so->server_root = NULL;
    so->server_signature = NULL;
    so->max_clients = 10;
    so->listen_port = 8000;
    so->daemon = 1;
    so->iterative = 0;

    while((read = getline(&line, &len, f)) != ERR) {
        // line is a comment
        if(line[0] == '#') {
            continue;
        }

        // line starts empty
        if(line[0] == '\n'
                || (line[0] == '\r' && line[1] == '\n')) {
            continue;
        }

        //                     v thanks, stackoverflow!
        if(sscanf(line, "%s = %[^\t\n]", option, rest) != 2) {
            // bad formatting
            return ERR;
        }

        // I wish C had strings, and they were good enough for switches
        if(strcmp(option, "server_root") == 0) {
            if(!(so->server_root = strdup(rest))) {
                // failed to get memory
                return ERR;
            }
            // let's get rid of those ugly ending /
            if(so->server_root[strlen(so->server_root) - 1] == '/') {
                so->server_root[strlen(so->server_root) - 1] = '\0';
            }
            continue;
        }
        if(strcmp(option, "server_signature") == 0) {
            if(!(so->server_signature = strdup(rest))) {
                // failed to get memory
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "max_clients") == 0) {
            if((so->max_clients = atoi(rest)) <= 0) {
                // this shit ain't right
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "listen_port") == 0) {
            if((so->listen_port = (uint16_t)atoi(rest)) <= 0) {
                // this shit ain't right
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "daemon") == 0) {
            if((so->daemon = atoi(rest)) < 0) {
                // this shit ain't right
                return ERR;
            }
            continue;
        }
        if(strcmp(option, "iterative") == 0) {
            if((so->iterative = atoi(rest)) < 0) {
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

void config_print(struct server_options *so) {
    if(!so) {
        return;
    }

    print("server_root: %s", so->server_root);
    print("server_signature: %s", so->server_signature);
    print("max_clients: %d", so->max_clients);
    print("listen_port: %d", so->listen_port);
}

// stackoverflow mention: https://stackoverflow.com/questions/2854488/reading-a-string-with-spaces-with-sscanf#2854510
