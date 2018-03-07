#ifndef _CGI_H
#define _CGI_H

#define MAX_SCRIPT_LINE_OUTPUT  256
#define TIMEOUT                 -4

long cgi_exec_script(const char *program, const char *resource, const char *input, int inlen, char **output, int wait_s);

#endif /*_CGI_H*/
