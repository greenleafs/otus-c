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
    htable_t *ht =  htable_make(8, NULL);
    if (!ht)
    {
        printf("hast table is not created!");
        return 1;
    }

    htable_item_base_t item;
    item.item_size = sizeof(htable_item_base_t);

    item.key = (uint8_t *)"FFFF";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"Сидели две вороны";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"Barbudos";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"FFFF";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"АШМУ";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"SIZ";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"Seven";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    item.key = (uint8_t *)"Eight";
    item.key_len = strlen((char*)item.key);
//    printf("KEY: %s ", item.key);
    htable_set(ht, &item);
//    printf("\n");

    htable_enumerate_items(ht, print_items);
    htable_destroy(ht);
    return 0;
}
