#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

void print_usage(const char *name)
{
    printf("Usage: %s zipjpeg [zipjpeg ...]\n", name);
}

#define ECDR_SIGNATURE 0x06054B50
#define ECDR_SIZE 22 // w/o padding; 24 - 2
typedef struct ecdr
{
    // 0x06054B50 -> 50 4B 05 06 (LE)
    uint32_t signature;
    // The number of this disk (containing
    // the end of central directory record)
    uint16_t disk_number;
    // Number of the disk on which the central directory starts
    uint16_t disk_start;
    // The number of central directory entries on this disk
    uint16_t disk_entries;
    // Total number of entries in the central directory.
    uint16_t total_cdr_entries;
    // Size of the central directory in bytes
    uint32_t cd_size;
    // Offset of the start of the central directory
    // on the disk on which the central directory starts
    uint32_t offset_cd_start_disk;
    // The length of the following comment field
    uint16_t comment_length;
    // PAD
    char pad[2];
} ecdr_t;


#define CDFH_SIGNATURE 0x02014B50
#define CDFH_SIZE 46
typedef struct cdfh
{
    // 0x02014B50 -> 50 4B 01 02 (LE)
    uint32_t signature;
    // Version
    uint16_t version;
    // version needed to extract
    uint16_t extract_version;
    // flags
    uint16_t flags;
    // compression method
    uint16_t compression_method;
    // file modification time
    uint16_t mod_time;
    // file modification date
    uint16_t mod_date;
    // crc-32 checksum
    uint32_t crc32;
    // compressed size
    uint32_t compressed_size;
    // uncompressed size
    uint32_t uncompressed_size;
    // file name length
    uint16_t name_length;
    // extra field length
    uint16_t extra_length;
    // file comment length;
    uint16_t comment_length;
    // the number of the disk on which this file exists
    uint16_t disk_start;
    // internal file attributes:
    uint16_t internal_attrs;
    // external file attributes - host-system dependent
    uint32_t external_attrs;
    // relative offset of local file header.
    uint32_t lfh_offset;
} cdfh_t;

static const char *find_ecdr_signature(const char *buff, size_t buff_size)
{
    const char *end_buffer = buff + buff_size;
    const char *q = end_buffer - sizeof(uint32_t);
    while (q >= buff)
    {
        if (*((uint32_t*)q) == ECDR_SIGNATURE)
        {
            return q;
        }
        q--;
    }
    return NULL;
}

#define BUFFER_SIZE (ECDR_SIZE + UINT16_MAX)
static bool find_ecdr(FILE *f, ecdr_t *ecdr)
{
    // get memory for buffer
    bool result = false;
    char *buff = (char *) malloc(BUFFER_SIZE);
    if (!buff)
    {
        perror("can't allocate memory in find_ecdr()");
        goto exit;
    }

    // set file position (from end)
    // ignore fseek error
    printf("%ud %d\n", BUFFER_SIZE, 0-BUFFER_SIZE);
    fseek(f, 0 - BUFFER_SIZE, SEEK_END);

    // read data from file
    size_t read_count = fread(buff, 1, BUFFER_SIZE, f);
    if (ferror(f))
    {
        perror("read error in find_ecdr()");
        goto cleanup;
    }

    // to find ECDR signature (as frame) in buffer
    // it isn't zip if ECDR signature don't find
    const char *found_ecdr = find_ecdr_signature(buff, read_count);
    if (found_ecdr)
    {
        memcpy(ecdr, found_ecdr, ECDR_SIZE);
        result = true;
        goto cleanup;
    }

cleanup:
    free(buff);
exit:
    return result;
}

static bool load_cdfh(FILE *f, long *cdfn_offset, cdfh_t *cdfh, long *name_offset)
{
    fseek(f, 0-*cdfn_offset, SEEK_END);
    fread(cdfh, CDFH_SIZE, 1, f);
    if (ferror(f))
    {
        perror("read error in load_cdfn()");
        return false;
    }
    if (cdfh->signature != CDFH_SIGNATURE)
    {
        return false;
    }

    *name_offset = *cdfn_offset - CDFH_SIZE;
    *cdfn_offset = *cdfn_offset -
                   CDFH_SIZE -
                   cdfh->name_length -
                   cdfh->extra_length -
                   cdfh->comment_length;
    return true;
}

static void read_and_print_file_name(FILE *f, long name_offset, size_t name_length)
{
    char *name_buffer = (char *)malloc(name_length + 1);
    if (!name_buffer) {
        perror("can't allocate memory in enumerate_files()");
        return;
    }

    fseek(f, 0-name_offset, SEEK_END);
    fread(name_buffer, name_length, 1, f);
    if (ferror(f))
    {
        perror("read error in enumerate_files()");
        goto cleanup;
    }

    name_buffer[name_length] = '\0';
    puts(name_buffer);
cleanup:
    free(name_buffer);
}

static void enumerate_files(FILE *f, long cdfh_start_offset)
{
    long name_offset;
    long cdfh_offset = cdfh_start_offset;
    cdfh_t cdfh;

    while (load_cdfh(f, &cdfh_offset, &cdfh, &name_offset))
    {
        read_and_print_file_name(f, name_offset, cdfh.name_length);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    FILE *f;
    ecdr_t ecdr;
    for (int i = 1; i < argc; i++)
    {
        printf("\nFile %s:\n", argv[i]);
        f = fopen(argv[i], "r");
        if (!f)
        {
            perror("Opening input file");
            puts("");
            continue;
        }

        bool res = find_ecdr(f, &ecdr);
        if (res)
        {
            printf("Files: %d\n", ecdr.disk_entries);
            enumerate_files(f, ecdr.comment_length + ECDR_SIZE + ecdr.cd_size);
        }
        else
        {
            puts("It isn't zip file or error happened while file was handling.");
        }

        puts("");
        fclose(f);
    }

    exit(EXIT_SUCCESS);
}
