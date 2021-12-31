#ifndef _H_TABLE_H
#define _H_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct htable_t htable_t;

typedef struct
{
    uint8_t *key;           // Pointer to a buffer with key
    size_t key_len;         // length of key's buffer
    size_t item_size;       // size of this structure
} htable_item_base_t;

typedef uint32_t (*hash_func_t)(const uint8_t *key, size_t key_len);
typedef void (*item_destructor_t)(htable_item_base_t *item);

/**
 *   Make a hash table
 *
 *   \param [in] init_len - The initial number of items in the hash table
 *
 *   \param [in] hash_func - The Pointer to your own hash function.
 *
 *   \return It returns pointer to the instance of hash table or NULL if error happened
 *
 *   \details The hash function takes a pointer to a key buffer, the length of the buffer,
 *   and returns the computed hash value for specified key.
 *   If NULL specified for the hashfunc parameter the default hash function will be used.
 */
htable_t* htable_make(size_t init_len, hash_func_t hash_func);


/**
 *  Destroy the hash table
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \details This function calls htable_item_destructor for every item.
 *  (see htable_set_item_destructor function description).
 */
void htable_destroy(htable_t *ht);

/**
 *  Insert item into the hash table
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] item - Pointer to a htable_item_base_t structure (or compatible structure)
 *
 *  \details If the key of item is not in hash table, it inserts the item in there. If the key of item
 *  exists in the hash table, the item will be changed and previous item will be destroyed.
 *  (see htable_set_item_destructor function description).
 *  This function makes a shallow copy of item so be careful with items that contains allocated resources!
 *  You can assume that the hash table takes ownership of the item.
 */
void htable_set(htable_t *ht, const htable_item_base_t *item);

/**
 *  Remove item from the hashtable
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] item - Pointer to a htable_item_base_t structure (or compatible)
 *
 *  \return It returns true if key has been found in hash table or false if it isn't so.
 *
 *  \details This function calls htable_item_destructor for removed item.
 *  (see htable_set_item_destructor function description)
 */
bool htable_remove(htable_t *ht, const htable_item_base_t *item);

/**
 *  Remove and return item from the hashtable
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] item - Pointer to a htable_item_base_t structure (or compatible)
 *
 *  \param [out] out_item - The pointer where the pointer to item will be returned
  *
 *  \return It returns true if key has been found in hash table or false if it isn't so.
 *
 *  \details This function doesn't call htable_item_destructor for removed item just returns it to out_item parameter.
 *  (see htable_set_item_destructor function description). You must free item itself and all
 *  associated resources yourself.
 */
bool htable_pop(htable_t *ht, const htable_item_base_t *item, htable_item_base_t **out_item);

/**
 *  Find and return item
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] item - Pointer to a htable_item_base_t structure (or compatible)
 *
 *  \param [out] out_item - The pointer where the pointer to item will be returned
 *
 *  \return  It returns true if key has been found in hash table or false if it isn't so.
 *
 *  \details You can specify NULL for out_item parameter if you  want just to check whether item is in hash table or not.
 *  Fields key, key_len, item_size of out_item structure must not be changed!!
 */
bool htable_find(htable_t *ht, const htable_item_base_t *item, htable_item_base_t **out_item);

/**
 * Enumerate keys of hash table
 *
 * \param [in] ht - The instance of hash table
 *
 * \param [in] callback - The pointer to function that will be called for each item in hash table
 *
 * \details The callback function can return zero to stop enumeration.
 * In other cases it has to return non-zero value.
 * Fields key, key_len, item_size of out_item structure must not be changed!!
 */
void htable_enumerate_items(htable_t *ht, int (*callback)(htable_item_base_t *item));

/**
 *  Set your own function that will be called when htable needs to delete item.
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] item_destructor - The pointer of your own item destructor
 *
 *  \returns This function returns pointer to item destructor set by previous call
 *
 *  \details The function can be called from htable_set, htable_remove, htable_destroy functions.
 *  You have to take care of releasing resources associated with item correctly and the item as well.
 *  By default the hash table just releases key buffer of item and releases item itself.
 *  Set item_destructor parameter to NULL to get back default item destructor
 */
item_destructor_t htable_set_item_destructor(htable_t *ht, item_destructor_t new_item_destructor);

typedef enum
{
    HTABLE_OK,
    HTABLE_MEM_ERROR,
    HTABLE_ITEM_SIZE_ERROR,
    HTABLE_UNKNOWN,
    HTABLE_FULL
} htable_status_t;

/**
 * Check the status of specified hash table
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \returns One of values of htable_status_t.
 */
 htable_status_t htable_status(htable_t *ht);

#endif // _H_TABLE_H
