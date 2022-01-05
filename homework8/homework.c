#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "htable.h"

void print_usage(const char *name)
{
    printf("Usage: %s textfile\n", name);
}

struct item
{
    htable_item_base_t base;
    size_t val;
};

int print_items(htable_item_base_t *item)
{
    struct item *p = (struct item*)item;
    printf("KEY: %s VAL: %lu\n", p->base.key, p->val);
    return 1;
}

int main(int argc, char *argv[])
{
    bool err_happened = false;

    if (argc < 2 || argc > 2)
    {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    int fd = open(argv[1], O_RDONLY);
    if (-1 == fd)
    {
        perror("Opening input file");
        return EXIT_FAILURE;
    }

    struct stat st;
    int res = fstat(fd, &st);
    if (-1 == res)
    {
        perror("Opening input file");
        err_happened = true;
        goto clean_file;        
    }

    char *file_content = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (file_content == MAP_FAILED)
    {
        perror("Map file to memory");
        err_happened = true;
        goto clean_file;
    }

    bool found;
    struct item item, *req_item;
    htable_t *ht = htable_make(128, NULL);
    if (!ht)
    {
        perror("Can't create hash table");
        err_happened = true;
        goto clean_file;
    }

    char *delimiters = " \n\t";
    char *token = strtok(file_content, delimiters);
    while (token) {
        item.base.key = (uint8_t*)token;
        item.base.key_len = strlen(token) + 1; // include zero-symbol
        item.val = 1;

        found = htable_find(ht, (htable_item_base_t*)&item, (htable_item_base_t**)&req_item);
        if (htable_status(ht) != HTABLE_OK)
        {
            perror("Something wrong when htable_find was called");
            err_happened = true;
            goto clean_hash_table;
        }

        if (found)
        {
            req_item->val++;
        }
        else
        {
            htable_set(ht, (htable_item_base_t*)&item, sizeof(struct item));
            if (htable_status(ht) != HTABLE_OK)
            {
                perror("Something wrong when htable_set was called");
                err_happened = true;
                goto clean_hash_table;
            }
        }

        token = strtok(NULL, delimiters);
    }

    htable_enumerate_items(ht, print_items);

clean_hash_table:
    htable_destroy(ht);

clean_file:
    close(fd);

    if (err_happened)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
