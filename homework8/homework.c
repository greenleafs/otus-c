#include <stdio.h>
#include <string.h>

#include "htable.h"

int print_items(htable_item_base_t *item)
{
    printf("Item key: %s\n", item->key);
    return 1;
}

int main(int argc, char *argv[])
{
    char *keys[] = {
        "FFFF", "Сидели две вороны", "Barbudos", "FFFF",
        "АШМУ", "SIZ", "Eight", "Seven", "Bubago",
        "Baranko", "Xeraxo", "Nidengo", "Mutengo", "Babango",
        "Samoras", "Reliefo", "Machanga", "Sixtatos", "Indigos"
    };

    htable_t *ht =  htable_make(8, NULL);
    if (!ht)
    {
        printf("hast table is not created!");
        return 1;
    }

    htable_item_base_t item;
    item.item_size = sizeof(htable_item_base_t);
    for(size_t i = 0; i < 5; i++)
    {
        item.key = (uint8_t *)keys[i];
        item.key_len = strlen((char*)item.key);
        htable_set(ht, &item);
    }

    htable_item_base_t *out;
    bool res;

    item.key = (uint8_t *)"АШМУ";
    item.key_len = strlen((char*)item.key);
    res = htable_find(ht, &item, &out);
    if (res)
    {
        printf("The key %s was found\n", item.key);
    } else {
        printf("The key %s was not found\n", item.key);
    }

    item.key = (uint8_t *)"Сидели две вороны";
    item.key_len = strlen((char*)item.key);
    res = htable_remove(ht, &item);
    if (res)
    {
        printf("The key %s was found\n", item.key);
    } else {
        printf("The key %s was not found\n", item.key);
    }

    item.key = (uint8_t *)"АШМУ";
    item.key_len = strlen((char*)item.key);
    res = htable_find(ht, &item, &out);
    if (res)
    {
        printf("The key %s was found\n", item.key);
    } else {
        printf("The key %s was not found\n", item.key);
    }

    htable_enumerate_items(ht, print_items);
    htable_destroy(ht);
    return 0;
}
