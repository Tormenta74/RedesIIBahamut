#include <stdio.h>
#include <string.h>
#include <regex.h>

#include "globals.h"
#include "scripts.h"

regex_t py, php;

int script_module_setup() {
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

int script_exec(const char *resource) {
    int status;
    char errbuf[128];

    if(!resource) {
        return ERR;
    }

    status = regexec(&py, resource, 0, NULL, 0);
    if(!status) {
        // match!
        // launch python interpreter
        print("python detected!");
        return OK;
    } else if(status != REG_NOMATCH) {
        regerror(status, &py, errbuf, 128);
        print("Regex (.py extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&php, resource, 0, NULL, 0);
    if(!status) {
        // match!
        // launch php interpreter
        print("php detected!");
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
