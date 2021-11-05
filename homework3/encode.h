#if !defined(_ENCODE_H_)
#define _ENCODE_H_

void print_supported_encodings(void);

typedef struct encoding_t encoding_t;
encoding_t* get_encoding_data(char *encoding_name);

int encode(FILE *in, FILE *out, encoding_t *enc);

#endif
