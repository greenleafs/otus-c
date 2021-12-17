#ifndef _H_TABLE_H
#define _H_TABLE_H

#include <stddef.h>

typedef struct htable_t htable_t;

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
 *   If NULL specified for the hashfunc parameter, default hash function will be used.
 */
htable_t* htable_make(size_t init_len, long (*hash_func)(const void *key, size_t key_size));

/**
 *  Make a hash table
 *
 *  \details It's a macro for call htable_make(8, NULL)
 */
#define htable htable_make(16, NULL);

/**
 *  Destroy the hash table
 *
 *  \param [in] ht - instance of hash table
 */
void htable_destroy(htable_t *ht);

/**
 *  Insert key into the hash table
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] key - The pointer to a buffer with key
 *
 *  \param [in] key_size - The length of the buffer
 *
 *  \param [in] data - The pointer to a buffer with data associated with this key
 *
 *  \param [in] data_size - The length of the buffer with data
 *
 *  \details If the key is not in hash table, it inserts key into there. If the key exists in hash table,
 *  only data associated with the key will be changed.
 */
void htable_set(htable_t *ht, const void *key, size_t key_size, const void *data, size_t data_size);

/**
 *  Remove key from the hashtable
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] key - The pointer to a buffer with key
 *
 *  \param [in] key_size - The length of the buffer
 *
 *  \param [out] data - The pointer where the pointer to the buffer with data associated with key will be returned
 *
 *  \param [out] data_size - The pointer where length of the buffer with data associated with the key will be returned.
 *  Specify NULL if you don't need that.
 *
 *  \return It returns 1 if key found in hash table or zero if it isn't so. Also see "data" and "data_size" parameters.
 *
 *  \details When a key is inserted into a hash table, a memory is allocated for data associated with specified key,
 * and this data is copied into this memory. When item with a particular key is removed from the hash table,
 * you can get data associated with the key using "data" parameter. After that You are the owner of the allocated
 * memory for data associated with removed key and you have to call free() function to release it.
 * Also, you can specify NULL for data parameter. In this case the allocated
 * memory for data associated with removed key will be automatically released.
 */
int htable_remove(htable_t *ht, const void *key, size_t key_size, void **data, size_t *data_size);

/**
 *  Find key and return data associated with it
 *
 *  \param [in] ht - The instance of hash table
 *
 *  \param [in] key - The pointer to a buffer with key
 *
 *  \param [in] key_size - The length of the buffer with key
 *
 *  \param [out] data - The pointer where the pointer to the buffer with data associated with key will be returned
 *
 *  \param [out] data_size - The pointer where length of the buffer with data associated with the key will be returned
 *  Specify NULL if you don't need that
 *
 *  \return It returns 1 if key found in hash table or zero if it isn't so. Also see "data" and "data_size" parameters.
 *
 *  \details You can specify NULL for "data" parameter if you only want to check that key is in hash table or not.
 */
int htable_find(htable_t *ht, const void *key, size_t key_size, const void **data, size_t *data_size);

/**
 *   Is there a key in the hash table
 *
 *    \details It's a macro for call htable_find(ht, key, key_size, NULL, NULL)
 */
#define has_key(ht, key, key_size) htable_find(ht, key, key_size, NULL, NULL)

/**
 * Enumerate keys of hash table
 *
 * \param [in] ht - The instance of hash table
 *
 * \param [in] callback - The pointer to function that will be called for each key in hash table
 *
 * \details The callback function function takes a next parameters: \code
 * ht - The pointer to the instance of hash table,
 * key - The pointer to a buffer with key
 * key_size - The length of the buffer with key
 * data - The pointer to a buffer with data associated with this key
 * data_size - The length of the buffer with data
 *
 * The callback function can return zero for stop enumeration. In other cases it have to return non-zero value.
 */
void htable_enumerate_keys(htable_t *ht, int (*callback)(htable_t *ht,
                                                         const void *key, size_t key_size,
                                                         const void *data, size_t data_size));


#endif // _H_TABLE_H
