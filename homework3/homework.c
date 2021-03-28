#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

#include "encode.h"


void print_usage(char *name)
{
    printf(
        "Convert text from some character encoding to utf-8"
        "\nUsage: %s -f from-encoding [-i input file] [-o output file]"
        "\n\t-f, --from-code=from-encoding - Use from-encoding for input characters."
        "\n\t-i, --input-file=file - Input file or stdin if not specified."
        "\n\t-o, --output-file=file - Output file or stdout if not specified."
        "\n\t-l, --list - List of supported encodings"
        "\n\t-h, --help - This help."
        "\n",
        name
    );
}


static struct option Options_[] = {
    {"help", no_argument, NULL, 'h'},
    {"list", no_argument, NULL, 'l'},
    {"from-code", required_argument, NULL, 'f'},
    {"input", required_argument, NULL, 'i'},
    {"output", required_argument, NULL, 'o'},
    {0}
};

struct givenOptions_
{
    char *input_file;
    char *output_file;
    char *from_enc;
    bool help;
    bool list;
};


int main(int argc, char *argv[])
{
    int opt, opt_index = 0;
    struct givenOptions_ given_options = {0};

    while ((opt = getopt_long(argc, argv, "hlf:i:o:", Options_, &opt_index)) != -1 )
    {
        switch (opt)
        {
        case 'f':
            given_options.from_enc = optarg;
            break;
        case 'i':
            given_options.input_file = optarg;
             break;
        case 'o':
            given_options.output_file = optarg;
            break;
        case 'l':
            given_options.list = true;
            break;
        case 'h':
            given_options.help = true;
            break;
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (given_options.help)
    {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (given_options.list)
    {
        print_supported_encodings();
        exit(EXIT_SUCCESS);
    }

    if (!given_options.from_enc)
    {
        printf("Option is required -- 'f'\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    encoding_t *enc = get_encoding_data(given_options.from_enc);
    if (!enc)
    {
        printf("Unsupported encoding -- %s\n", given_options.from_enc);
        print_supported_encodings();
        exit(EXIT_FAILURE);
    }

    FILE *in = stdin;
    if (given_options.input_file)
    {
        in = fopen(given_options.input_file, "rb");
        if (!in)
        {
            perror(given_options.input_file);
            exit(EXIT_FAILURE);
        }
    }

    FILE *out = stdout;
    if (given_options.output_file)
    {
        out = fopen(given_options.output_file, "wb");
        if (!out)
        {
            perror(given_options.output_file);
            fclose(in);
            exit(EXIT_FAILURE);
        }
    }

    int err = encode(in, out, enc);

    fclose(in);
    fclose(out);

    if (!err)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}
