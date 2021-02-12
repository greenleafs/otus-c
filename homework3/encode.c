#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "encode.h"

uint32_t koi8_r[] = {
    0, 1, 2
};

uint32_t cp1251[] = {
    0, 1, 2
};

uint32_t iso_8859_5[] = {
    0, 1, 2
};


typedef struct _encoding
{
    char *encoding_name;
    uint32_t *encoding_data;
} encoding;


static encoding *supported_encodings[] = {
    &(encoding){"koi8-r", koi8_r},
    &(encoding){"cp1251", cp1251},
    NULL
};


typedef struct _utf8 {
    char mask;
    char lead;
    uint32_t start_range;
    uint32_t end_range;
} utf8;

void print_supported_encodings(void)
{
    printf("Supported encodings:");
    for(encoding **e = supported_encodings; *e; ++e) {
        printf("\n\t%s", (*e)->encoding_name);
    }
    puts("");
}

encoding *get_encoding_data(char *encoding_name)
{
    for( encoding **e = supported_encodings; *e; ++e) {
        if( !strcmp( encoding_name, (*e)->encoding_name ))
            return *e;
    }
    return NULL;
}

