#ifndef _FINDER_H
#define _FINDER_H

#define NO_MATCH -1
#define NO_CTYPE -2

int finder_setup();
void finder_clean();
long finder_load(const char *resource, const char *input, int inlen, char **output, char **contenttype);

#endif /*_FINDER_H*/
