#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>

typedef struct server_options {
    char *server_root;          // directory
    char *server_signature;     // server name
    int max_clients;
    uint16_t listen_port;
    int daemon;                 // default: 0
    int iterative;              // default: 0
} server_options_t;

int config_parse(char* filename, struct server_options *so);
void config_free(struct server_options *so);
void config_print(struct server_options *so);

#endif /*_CONFIG_H*/
