#ifndef _CGI_H
#define _CGI_H

#define NO_MATCH 666

#define MAX_SCRIPT_LINE_OUTPUT 256

int cgi_module_setup();
void cgi_module_clean();
int cgi_exec_script(const char *resource, const char *input, int inlen, char **output);
//int cgi_fork_exec(const char *program, const char *resource, const char *input, int len, char **output);

#endif /*_CGI_H*/
