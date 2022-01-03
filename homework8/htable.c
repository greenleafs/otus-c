#include <stdlib.h>
#include <string.h>

#include "htable.h"

#define CHECK_AND_EXIT_WITH_VAL_IF(cond, v)       if (cond)  return v;
#define CHECK_AND_EXIT_IF(cond)                   if (cond)  return;
#define SET_HTABLE_ERROR_AND_EXIT_IF(cond, h, e)  \
    if (cond)                                     \
    {                                             \
      h->last_error = e;                          \
      return;                                     \
    }                                             \

#define SET_HTABLE_ERROR_AND_EXIT_WITH_VAL_IF(cond, h, e, v)  \
    if (cond)                                                 \
    {                                                         \
      h->last_error = e;                                      \
      return v;                                               \
    }                                                         \

struct htable_t
{
    htable_item_base_t **items;
    size_t items_count;
    size_t capacity;
    hash_func_t hash_func;
    item_destructor_t item_destructor;
    int last_error;
};

// We use address of this for marking items as deleted
htable_item_base_t deleted__;
htable_item_base_t *marked_as_deleted = &deleted__;

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
    CHECK_AND_EXIT_IF(new_capacity < ht->capacity) // overflow control.

    htable_item_base_t **new_items = calloc(new_capacity, sizeof(htable_item_base_t*));
    CHECK_AND_EXIT_IF(!new_items)

    htable_item_base_t *item;
    uint32_t hash;
    size_t index;
    for (size_t i = 0; i < ht->capacity; i++)
    {
        item = ht->items[i];
        if (item && item != marked_as_deleted)
        {
            hash = ht->hash_func(item->key, item->key_len);
            index = hash % new_capacity;
            while (new_items[index])
                index++, index %= new_capacity;
            new_items[index] = item;
        }
    }

    free(ht->items);
    ht->items = new_items;
    ht->capacity = new_capacity;
}

static bool find(htable_t *ht, const htable_item_base_t *item, size_t *out_index)
{
    uint32_t hash = ht->hash_func(item->key, item->key_len);
    size_t index = hash % ht->capacity;
    htable_item_base_t *candidate = ht->items[index];

    if (!candidate)
    {
        *out_index = index;
        return false;
    }

    size_t stopper = index;
    while (candidate)
    {
        if (candidate != marked_as_deleted && compare_items_key(candidate, item))
        {
            *out_index = index;
            return true;
        }

        candidate = ht->items[++index % ht->capacity];
        SET_HTABLE_ERROR_AND_EXIT_WITH_VAL_IF(index % ht->capacity == stopper, ht, HTABLE_FULL, false)
    }

    *out_index = index;
    return false;
}

inline htable_status_t htable_status(htable_t *ht)
{
    CHECK_AND_EXIT_WITH_VAL_IF(!ht, HTABLE_UNKNOWN)
    return ht->last_error;
}

item_destructor_t htable_set_item_destructor(htable_t *ht, item_destructor_t new_item_destructor)
{
    CHECK_AND_EXIT_WITH_VAL_IF(!ht, NULL)
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
    struct htable_t *htable = malloc(sizeof(struct htable_t));
    CHECK_AND_EXIT_WITH_VAL_IF(!htable, NULL)

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
    CHECK_AND_EXIT_IF(!ht)
    htable_item_base_t *item;

    for (size_t i = 0; i < ht->capacity; i++)
    {
        item = ht->items[i];
        if (item && item != marked_as_deleted)
            ht->item_destructor(ht->items[i]);
    }

    free(ht->items);
    free(ht);
}

void htable_set(htable_t *ht, const htable_item_base_t *item, size_t item_size)
{
    CHECK_AND_EXIT_IF(!ht)
    CHECK_AND_EXIT_IF(!item)
    CHECK_AND_EXIT_IF(!item->key)
    CHECK_AND_EXIT_IF(item_size < sizeof(htable_item_base_t))

    if (ht->items_count > (ht->capacity >> 1))
        expand(ht);  // Expand && Rehash all items

    size_t index;
    bool is_founded = find(ht, item, &index);
    CHECK_AND_EXIT_IF(ht->last_error == HTABLE_FULL)

    if (is_founded)
    {
       ht->item_destructor(ht->items[index]);
       ht->items_count--;
    }

    htable_item_base_t *candidate = (htable_item_base_t*)calloc(1, item_size);
    SET_HTABLE_ERROR_AND_EXIT_IF(!candidate, ht, HTABLE_MEM_ERROR)

    memcpy(candidate, item, item_size);
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
    CHECK_AND_EXIT_IF(!ht)
    CHECK_AND_EXIT_IF(!callback)
    int callback_result;
    htable_item_base_t *item;

    for (size_t i = 0; i < ht->capacity; i++)
    {
        item = ht->items[i];
        if (item && item != marked_as_deleted)
        {
            callback_result = callback(ht->items[i]);
            if( !callback_result )
                break;
        }
    }
}

bool htable_find(htable_t *ht, const htable_item_base_t *item, htable_item_base_t **out_item)
{
    CHECK_AND_EXIT_WITH_VAL_IF(!ht, false)
    CHECK_AND_EXIT_WITH_VAL_IF(!item, false)
    CHECK_AND_EXIT_WITH_VAL_IF(!item->key, false)

    size_t index;
    bool is_founded = find(ht, item, &index);

    if (is_founded)
    {
        if (out_item)
            *out_item = ht->items[index];
        return true;
    }
    return false;
}

bool htable_remove(htable_t *ht, const htable_item_base_t *item)
{
    CHECK_AND_EXIT_WITH_VAL_IF(!ht, false)
    CHECK_AND_EXIT_WITH_VAL_IF(!item, false)
    CHECK_AND_EXIT_WITH_VAL_IF(!item->key, false)

    size_t index;
    bool is_founded = find(ht, item, &index);
    if (!is_founded)
        return false;

    ht->item_destructor(ht->items[index]);
    ht->items[index] = marked_as_deleted;
    ht->last_error = HTABLE_OK;
    return true;
}

htable_item_base_t *htable_pop(htable_t *ht, const htable_item_base_t *item)
{
    CHECK_AND_EXIT_WITH_VAL_IF(!ht, false)
    CHECK_AND_EXIT_WITH_VAL_IF(!item, false)
    CHECK_AND_EXIT_WITH_VAL_IF(!item->key, false)

    size_t index;
    bool is_founded = find(ht, item, &index);
    if (!is_founded)
        return NULL;

    htable_item_base_t *res = ht->items[index];
    ht->items[index] = marked_as_deleted;
    ht->last_error = HTABLE_OK;
    return res;
}
