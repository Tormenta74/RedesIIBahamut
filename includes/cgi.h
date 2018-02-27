#ifndef _CGI_H
#define _CGI_H

#define MAX_SCRIPT_LINE_OUTPUT 256

long cgi_exec_script(const char *program, const char *resource, const char *input, int inlen, char **output);

#endif /*_CGI_H*/
