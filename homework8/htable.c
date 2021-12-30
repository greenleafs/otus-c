#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "htable.h"

#define CHECK_AND_EXIT_WITH_VAL(x, v)  if (!x)  return v;
#define CHECK_AND_EXIT(x)              if (!x)  return;
#define CHECK_ITEM_SIZE_AND_EXIT(h, x) if (x->item_size < sizeof(htable_item_base_t)) \
                                       {                                              \
                                           h->last_error = HTABLE_ITEM_SIZE_ERROR;    \
                                           return;                                    \
                                       }
#define CHECK_MEM_ERROR_AND_EXIT(h, x) if (!x)                                        \
                                       {                                              \
                                           h->last_error = HTABLE_MEM_ERROR;          \
                                           return;                                    \
                                       }

struct htable_t
{
    htable_item_base_t **items;
    size_t items_count;
    size_t capacity;
    hash_func_t hash_func;
    item_destructor_t item_destructor;
    int last_error;
};

/**
 * Jenkins hash function. Used as default hash function.
 * see: https://en.wikipedia.org/wiki/Jenkins_hash_function
 */
static uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, size_t key_len)
{
    size_t i = 0;
    uint32_t hash = 0;

    while (i != key_len)
    {
        hash += key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

static void default_item_destructor(htable_item_base_t *item)
{
    free(item->key);
    free(item);
}

static bool compare_items_key(const htable_item_base_t *item_first, const htable_item_base_t *item_second)
{
    if (item_first->key_len != item_second->key_len)
        return false;

    if (0 != memcmp(item_first->key, item_second->key, item_first->key_len))
        return false;

    return true;
}

static void expand(htable_t *ht)
{
    size_t new_capacity = ht->capacity << 1;
    // overflow control.
    if (new_capacity < ht->capacity)
    {
        ht->last_error = HTABLE_MEM_ERROR;
        return;
    }

    htable_item_base_t **new_items = calloc(new_capacity, sizeof(htable_item_base_t*));
    CHECK_MEM_ERROR_AND_EXIT(ht, new_items)

    htable_item_base_t *item;
    uint32_t hash;
    size_t index;
    for (size_t i = 0; i < ht->capacity; i++)
    {
        item = ht->items[i];
        if (item)
        {
            hash = ht->hash_func(item->key, item->key_len);
            index = hash % new_capacity;
            while (new_items[index] != NULL)
                index++, index %= new_capacity;
            new_items[index] = item;
        }
    }

    free(ht->items);
    ht->items = new_items;
    ht->capacity = new_capacity;
}

inline htable_status_t htable_status(htable_t *ht)
{
    CHECK_AND_EXIT_WITH_VAL(ht, HTABLE_UNKNOWN)
    return ht->last_error;
}

item_destructor_t htable_set_item_destructor(htable_t *ht, item_destructor_t new_item_destructor)
{
    CHECK_AND_EXIT_WITH_VAL(ht, NULL)
    item_destructor_t current_item_destructor = ht->item_destructor;
    if (new_item_destructor == NULL)
    {
        ht->item_destructor = default_item_destructor;
    }
    else
    {
        ht->item_destructor = new_item_destructor;
    }
    return current_item_destructor;
}

htable_t* htable_make(size_t init_len, uint32_t (*hash_func)(const uint8_t *key, size_t key_len))
{
    struct htable_t *htable = calloc(1, sizeof(struct htable_t));
    CHECK_AND_EXIT_WITH_VAL(htable, NULL)

    size_t capacity = (init_len < 8) ? 8 : init_len;
    htable->items = calloc(capacity, sizeof(htable_item_base_t*));
    if ( !htable->items )
    {
        free(htable);
        return NULL;
    }

    htable->hash_func = (hash_func) ? hash_func : jenkins_one_at_a_time_hash;
    htable->item_destructor = default_item_destructor;
    htable->capacity = capacity;
    htable->items_count = 0;
    htable->last_error = HTABLE_OK;
    return htable;
}

void htable_destroy(htable_t *ht)
{
    CHECK_AND_EXIT(ht)

    for (size_t i = 0; i < ht->capacity; i++)
    {
        if (ht->items[i])
            ht->item_destructor(ht->items[i]);
    }

    free(ht->items);
    free(ht);
}

void htable_set(htable_t *ht, const htable_item_base_t *item)
{
    CHECK_AND_EXIT(ht)
    CHECK_AND_EXIT(item)
    CHECK_ITEM_SIZE_AND_EXIT(ht, item)

    if (ht->items_count > (ht->capacity >> 1))
    {
        // Expand && Rehash all items
        printf("Need to expand\n");
        expand(ht);
    }

    uint32_t hash = ht->hash_func(item->key, item->key_len);
    size_t index = hash % ht->capacity;
    htable_item_base_t *candidate = ht->items[index];
    if (candidate != NULL)
    {
        if (compare_items_key(candidate, item))
        {
            ht->item_destructor(candidate);
            ht->items_count--;
        }
        else
        {
            // Resolve collision (linear probing).
            printf("Collision found %X\n", hash);
            while (candidate != NULL)
                candidate = ht->items[++index % ht->capacity];
        }
    }

    candidate = (htable_item_base_t*)calloc(1, item->item_size);
    CHECK_MEM_ERROR_AND_EXIT(ht, candidate)

    memcpy(candidate, item, item->item_size);
    candidate->key = calloc(1, candidate->key_len);
    if (!candidate->key)
    {
        free(candidate);
        ht->last_error = HTABLE_MEM_ERROR;
        return;
    }

    memcpy(candidate->key, item->key, candidate->key_len);
    ht->items[index] = candidate;
    ht->items_count++;
    ht->last_error = HTABLE_OK;
}

void htable_enumerate_items(htable_t *ht, int (*callback)(htable_item_base_t *item))
{
    CHECK_AND_EXIT(ht)
    CHECK_AND_EXIT(callback)
    int callback_result;
    for (size_t i = 0; i < ht->capacity; i++)
    {
        if (ht->items[i])
        {
            callback_result = callback(ht->items[i]);
            if( !callback_result )
                break;
        }
    }

    ht->last_error = HTABLE_OK;
}
