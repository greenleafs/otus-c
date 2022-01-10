#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

    printf("%.*s %lu\n", (int)p->base.key_len, p->base.key, p->val);
    return 1;
}

int main(int argc, char *argv[])
{
    bool err_happened = false;

    if (argc < 2 || argc > 2)
    {
        print_usage(argv[0]);
        goto quit;
    }

    int fd = open(argv[1], O_RDONLY);
    if (-1 == fd)
    {
        perror("Opening input file");
        err_happened = true;
        goto quit;
    }

    struct stat st;
    int res = fstat(fd, &st);
    if (-1 == res)
    {
        perror("Opening input file");
        err_happened = true;
        goto clean_file;        
    }

    if (!st.st_size)
        goto quit;

    uint8_t *file_content = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (file_content == MAP_FAILED)
    {
        perror("Map file to memory");
        err_happened = true;
        goto clean_file;
    }

    bool found;
    struct item item, *req_item;
    htable_t *ht = htable_make(512, NULL);
    if (!ht)
    {
        perror("Can't create hash table");
        err_happened = true;
        goto clean_file;
    }

    int64_t index = 0;
    uint8_t *p = file_content;
    uint8_t *word;
    size_t word_len;
    while (index < st.st_size)
    {
        while ((*p == ' ' || *p == '\n' || *p == '\t') && index < st.st_size)
        {
            p++; index++;
        }

        if (index >= st.st_size)
            break;

        word = p;
        word_len = 0;
        while ((*p != ' ' && *p != '\n' && *p != '\t') && index < st.st_size)
        {
            p++; index++; word_len++;
        }

        item.base.key = word;
        item.base.key_len = word_len;
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
    }

    htable_enumerate_items(ht, print_items);

clean_hash_table:
    htable_destroy(ht);
    munmap(file_content, st.st_size);

clean_file:
    close(fd);

quit:
    if (err_happened)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
