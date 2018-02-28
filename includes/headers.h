#ifndef _HEADERS_H
#define _HEADERS_H

char *header_date();
char *header_server();
char *header_last_modified();
char *header_content_length();
char *header_content_type();
char *response_craft(int version, char *content);

//nope
char *date_generator(time_t *t);

#endif /*_HEADERS_H*/
