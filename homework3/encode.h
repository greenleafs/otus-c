#ifndef _ENCODE_H_
#define _ENCODE_H_

void print_supported_encodings( void );

typedef struct _encoding encoding;
encoding *get_encoding_data(char *encoding_name);

int encode(FILE *in, FILE *out, encoding *enc);

#endif