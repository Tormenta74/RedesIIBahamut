#ifndef _FINDER_H
#define _FINDER_H

#define NOT_FOUND -2
#define NO_MATCH -3

int finder_setup();
void finder_clean();
long finder_load(const char *resource, const char *input, int inlen, void **output, char **contenttype, int *check_flag);

#endif /*_FINDER_H*/
