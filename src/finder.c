#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <regex.h>

#include "globals.h"
#include "cgi.h"
#include "finder.h"

extern int tout_seconds;    // server.c

// all the file extensions described in the requirements
regex_t txt;
regex_t html, htm;
regex_t gif;
regex_t jpeg, jpg;
regex_t png;
regex_t mpeg, mpg;
regex_t doc, docx;
regex_t pdf;
regex_t py, php;

/*
 * Description: Allocates and initializes the regular expressions to analyze the file types.
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
int finder_setup() {
    int status;

    status = regcomp(&doc, "^.*\\.doc$", 0);
    if (status) {
        print("finder: Could not compile the doc extension regex.");
        return ERR;
    }

    status = regcomp(&docx, "^.*\\.docx$", 0);
    if (status) {
        print("Could not compile the docx extension regex.");
        regfree(&doc);
        return ERR;
    }

    status = regcomp(&gif, "^.*\\.gif$", 0);
    if (status) {
        print("finder: Could not compile the gif extension regex.");
        regfree(&doc);
        regfree(&docx);
        return ERR;
    }

    status = regcomp(&htm, "^.*\\.htm$", 0);
    if (status) {
        print("finder: Could not compile the htm extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        return ERR;
    }

    status = regcomp(&html, "^.*\\.html$", 0);
    if (status) {
        print("finder: Could not compile the html extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        return ERR;
    }

    status = regcomp(&jpeg, "^.*\\.jpeg$", 0);
    if (status) {
        print("finder: Could not compile the jpeg extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        return ERR;
    }

    status = regcomp(&jpg, "^.*\\.jpg$", 0);
    if (status) {
        print("finder: Could not compile the jpg extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        return ERR;
    }

    status = regcomp(&mpeg, "^.*\\.mpeg$", 0);
    if (status) {
        print("finder: Could not compile the mpeg extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        return ERR;
    }

    status = regcomp(&mpg, "^.*\\.mpg$", 0);
    if (status) {
        print("finder: Could not compile the mpg extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        regfree(&mpeg);
        return ERR;
    }

    status = regcomp(&pdf, "^.*\\.pdf$", 0);
    if (status) {
        print("finder: Could not compile the pdf extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        regfree(&mpeg);
        regfree(&mpg);
        return ERR;
    }

    status = regcomp(&php, "^.*\\.php$", 0);
    if (status) {
        print("finder: Could not compile the php extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        regfree(&mpeg);
        regfree(&mpg);
        regfree(&pdf);
        return ERR;
    }

    status = regcomp(&png, "^.*\\.png$", 0);
    if (status) {
        print("finder: Could not compile the png extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        regfree(&mpeg);
        regfree(&mpg);
        regfree(&pdf);
        regfree(&php);
        return ERR;
    }

    status = regcomp(&py, "^.*\\.py$", 0);
    if (status) {
        print("finder: Could not compile the Python extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        regfree(&mpeg);
        regfree(&mpg);
        regfree(&pdf);
        regfree(&php);
        regfree(&png);
        return ERR;
    }


    status = regcomp(&txt, "^.*\\.txt$", 0);
    if (status) {
        print("finder: Could not compile the txt extension regex.");
        regfree(&doc);
        regfree(&docx);
        regfree(&gif);
        regfree(&htm);
        regfree(&html);
        regfree(&jpeg);
        regfree(&jpg);
        regfree(&mpeg);
        regfree(&mpg);
        regfree(&pdf);
        regfree(&php);
        regfree(&png);
        regfree(&py);
        return ERR;
    }

    return OK;
}

/*
 * Description: Deallocates the regular expressions.
 */
void finder_clean() {
    regfree(&doc);
    regfree(&docx);
    regfree(&gif);
    regfree(&htm);
    regfree(&html);
    regfree(&jpeg);
    regfree(&jpg);
    regfree(&mpeg);
    regfree(&mpg);
    regfree(&pdf);
    regfree(&php);
    regfree(&png);
    regfree(&py);
    regfree(&txt);
}

// note: if
/*
 * Description: Allocates memory for a buffer containing the Content-Type string.
 * (if the string contains spaces, this function may cause problems)
 *
 * In:
 * const char *type: literal string
 * char **contenttype: pointer to the memory address to be reserved
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
int fill_content_type(const char *type, char **contenttype) {
    if (!type || !contenttype) {
        return ERR;
    }

    *contenttype = (char*)malloc((strlen(type) + 1) * sizeof(char));
    if (!*contenttype) {
        return ERR;
    }

    sprintf(*contenttype, "%s", type);
    //*contenttype[strlen(type)] = '\0';

    return OK;
}

/*
 * Description: Attempts to find the file or script in the "resource" path and then
 * determine it's filetype based on the filename extension in this fashion:
 *
 * text/plain: .txt, .php, .py
 * text/html: .html, .htm
 * image/gif: .gif
 * image/png: .png
 * image/jpeg: .jpeg, .jpg
 * video/mpeg: .mpeg, .mpg
 * application/msword: .doc, .docx
 * application/pdf: .pdf
 *
 * In:
 * int wait_s: desired timeout in seconds
 *
 * Return: ERR in case of failure at any point. NOT_FOUND if the resource does not exist
 * or is a directory. NO_MATCH if the filetype could not be determined. TIMEOUT if the resource
 * is a script and execution took longer than the specified value in seconds. Size of the output
 * string otherwise.
 */
long finder_load(const char *resource, const char *input, int inlen, void **output, char **contenttype, int *check_flag) {
    int status;
    char errbuf[128];

    FILE *file_pointer;
    long file_len;

    if (!resource || !output || !contenttype) {
        return ERR;
    }

    *check_flag = 0;

    // first check if it's a script

    status = regexec(&php, resource, 0, NULL, 0);
    if (!status) {
        // launch fork_exec php
        char *script_output;
        if ((file_len = cgi_exec_script("php", resource, input, inlen, &script_output, tout_seconds)) == ERR) {
            // failed to execute
            return ERR;
        }
        if (file_len == TIMEOUT) {
            // TODO: propagate timeout and communicate 500 internal server error
            return TIMEOUT;
        }
        if (fill_content_type("text/plain", contenttype) != ERR) {
            *check_flag = 1;
        }
        *output = script_output;
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &php, errbuf, 128);
        print("finder: Regex (.php extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&py, resource, 0, NULL, 0);
    if (!status) {
        // launch fork_exec python
        char *script_output;
        if ((file_len = cgi_exec_script("python", resource, input, inlen,  &script_output, tout_seconds)) == ERR) {
            // failed to execute
            return ERR;
        }
        if (file_len == TIMEOUT) {
            // TODO: propagate timeout and communicate 500 internal server error
            return TIMEOUT;
        }
        if (fill_content_type("text/plain", contenttype) != ERR) {
            *check_flag = 1;
        }
        *output = script_output;
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &py, errbuf, 128);
        print("finder: Regex (.py extension) match failed: %s", errbuf);
        return ERR;
    }

    // check that we did not request a directory

    struct stat s;
    if (stat(resource, &s) == OK) {
        if (s.st_mode & S_IFDIR) {
            print("finder: %s is a directory", resource);
            return NOT_FOUND;
        }
    } else {
        print("finder: Could not stat file %s", resource);
        return ERR;
    }

    // if not, attempt to open the file

    if ((file_pointer = fopen(resource, "r")) == NULL) {
        print("finder: Could not read file %s", resource);
        return NOT_FOUND;
    }

    // calculate file size
    fseek(file_pointer, 0, SEEK_END);
    file_len = ftell(file_pointer);
    fseek(file_pointer, 0, SEEK_SET);

    // reserve enough memory for the entire file and load it
    *output = malloc(file_len);
    fread(*output, (size_t)file_len, 1, file_pointer);

    // done with the file pointer
    fclose(file_pointer);

    ////////////////////////////////////////////////////
    // Execute each of the regular expressions on the //
    // filename, with the respective error checks and //
    // filling of the content type.                   //
    ////////////////////////////////////////////////////

    status = regexec(&txt, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("text/plain", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &txt, errbuf, 128);
        print("finder: Regex (.txt extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&html, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("text/html", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &html, errbuf, 128);
        print("finder: Regex (.html extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&htm, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("text/html", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &htm, errbuf, 128);
        print("finder: Regex (.htm extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&gif, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("image/gif", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &gif, errbuf, 128);
        print("finder: Regex (.gif extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&png, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("image/png", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &png, errbuf, 128);
        print("finder: Regex (.png extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&jpeg, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("image/jpeg", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &jpeg, errbuf, 128);
        print("finder: Regex (.jpeg extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&jpg, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("image/jpeg", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &jpg, errbuf, 128);
        print("finder: Regex (.jpg extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&mpeg, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("video/mpeg", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &mpeg, errbuf, 128);
        print("finder: Regex (.mpeg extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&mpg, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("video/mpeg", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &mpg, errbuf, 128);
        print("finder: Regex (.mpg extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&docx, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("application/msword", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &docx, errbuf, 128);
        print("finder: Regex (.docx extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&doc, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("application/msword", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &doc, errbuf, 128);
        print("finder: Regex (.doc extension) match failed: %s", errbuf);
        return ERR;
    }

    status = regexec(&pdf, resource, 0, NULL, 0);
    if (!status) {
        if (fill_content_type("application/pdf", contenttype) != ERR) {
            *check_flag = 1;
        }
        return file_len;
    } else if (status != REG_NOMATCH) {
        regerror(status, &pdf, errbuf, 128);
        print("finder: Regex (.pdf extension) match failed: %s", errbuf);
        return ERR;
    }

    // no match at all
    print("finder: Resource does not match any of the supported scripts.");
    return NO_MATCH;
}
