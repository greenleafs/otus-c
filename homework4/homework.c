#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

void print_usage(const char *name)
{
    printf("Usage: %s zipjpeg [zipjpeg ...]\n", name);
}

#define ECDR_SIGNATURE 0x06054B50
#define ECDR_SIZE 22 // w/o padding; 24 - 2
typedef struct _ecdr
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
typedef struct _cdfh {
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

typedef struct _find_result {
    size_t offset;
    int8_t result;
} find_result_t;


static find_result_t _find_frame(const char *buff, size_t buff_size, uint32_t val)
{
    find_result_t res = { 0, 0 };

    const char *end_buffer = buff + buff_size;
    const char *q = end_buffer - sizeof(val);
    while ( q >= buff ) {
        if( *((uint32_t*)q) == val ){
            res.offset = end_buffer - q;
            res.result = 1;
            return res;
        }
        q--;
    }
    return res;
}

#define BUFFER_SIZE ((size_t)ECDR_SIZE + (size_t)UINT16_MAX)
static bool find_ecdr_offset(FILE *f, size_t *offset)
{
    // offset pointer defence
    if( !offset )
        return false;

    // get memory for buffer
    char *buff = (char *) malloc( BUFFER_SIZE );
    if( !buff ) {
        perror( "ecdr_offset" );
        return false;
    }

    bool result = false;

    // set file position (from end)
    // ignore fseek error
    fseek( f, 0 - BUFFER_SIZE, SEEK_END );

    // read data from file
    size_t readed = fread( buff, 1, BUFFER_SIZE, f );
    if( ferror( f )) {
        perror( "read error" );
        goto cleanup;
    }

    // to find ECDR signature (as frame) in buffer
    // it isn't zip if it don't find
    find_result_t fres = _find_frame( buff, readed, ECDR_SIGNATURE );
    if( fres.result ) {
        *offset = fres.offset;
        result = true;
        goto cleanup;
    }

cleanup:
    free( buff );
    return result;
}

static bool load(void *dest, size_t size, FILE *f, size_t offset )
{
    fseek( f, 0 - offset, SEEK_END );
    fread( dest, size, 1, f );
    if( ferror( f )) {
        perror( "load ecdr" );
        return false;
    }
    return true;
}

static bool load_cdfh(FILE *f, size_t offset, cdfh_t *cdfh, size_t *name_offset, size_t *next_cdfn_offset)
{
    if( !cdfh )
        return false;
    if( !next_cdfn_offset )
        return false;
    if( !name_offset )
        return false;

    if( !load( cdfh, CDFH_SIZE, f, offset )) {
        return false;
    }
    if( cdfh->signature != CDFH_SIGNATURE )
        return false;

    *name_offset = offset - CDFH_SIZE;
    *next_cdfn_offset = offset -
                        CDFH_SIZE -
                        cdfh->name_length -
                        cdfh->extra_length -
                        cdfh->comment_length;
    return true;
}

static void enumerate_files(FILE *f, size_t cdfh_start_offcet)
{
    size_t name_offset;
    size_t next_cdfh_offcet = cdfh_start_offcet;
    cdfh_t cdfh;

    while( load_cdfh( f, next_cdfh_offcet, &cdfh, &name_offset, &next_cdfh_offcet )) {
        char *name_buffer = (char *)malloc( cdfh.name_length + 1 );
        if( load( name_buffer, cdfh.name_length, f, name_offset )) {
            name_buffer[ cdfh.name_length ] = '\0';
            puts( name_buffer );
        }
        else {
            puts( "Something went wrong. Cant detect file name in cdfh." );
        }

        free( name_buffer );
    }
}

int main(int argc, char *argv[])
{
    if( argc < 2)
    {
        print_usage( argv[0] );
        exit( EXIT_SUCCESS );
    }

    size_t ecdr_offset;
    ecdr_t ecdr;

    FILE *f;
    for( int i = 1; i < argc; i++ ) {
        printf( "\nFile %s:\n", argv[i] );
        f = fopen( argv[i], "r" );
        if( !f ) {
            perror( NULL );
            puts( "" );
            continue;
        }

        bool res = find_ecdr_offset( f, &ecdr_offset );
        if( res ) {
            if( !load( &ecdr, ECDR_SIZE,  f, ecdr_offset ))
                continue;

            printf( "Files: %d\n", ecdr.disk_entries );
            enumerate_files( f, ecdr.comment_length + ECDR_SIZE + ecdr.cd_size );
        }
        else
            puts( "It isn't zip file." );

        puts( "" );
    }

    exit( EXIT_SUCCESS );
}
