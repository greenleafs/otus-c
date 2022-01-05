#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/// --------------------- PRIMITIVE TEST SYSTEM --------------------

// MOCKS FOR MEMORY ALLOCATE/FREE FUNCTIONS
#define MEM_SIZE 4096
#define MEMOP_TRACK_SIZE 256
#define GUARD_SIZE 16
#define GUARD_BYTE 0xff

struct allocated
{
    uint8_t *underflow_guard;
    uint8_t *mem;
    uint8_t *overflow_guard;
    char *file;
    int line;
    size_t size;
};

struct freed
{
    uint8_t *mem;
    char *file;
    int line;
};

uint8_t mem[MEM_SIZE];                             // Mock memory buffer
struct freed mem_freed[MEMOP_TRACK_SIZE];          // Track freed pointers
struct allocated mem_allocated[MEMOP_TRACK_SIZE];  // Track allocated pointers

size_t mem_next_idx = 0;
size_t freed_next_idx = 0;
size_t allocated_next_idx = 0;
int allocation_error_emulation_flag = 0;
size_t allocation_error_emulation_after_nth_calls = 0;

void reset_mem()
{
    memset(mem, 0, MEM_SIZE);
    memset(mem_freed, 0, MEMOP_TRACK_SIZE * sizeof(struct freed));
    memset(mem_allocated, 0, MEMOP_TRACK_SIZE * sizeof(struct allocated));
    mem_next_idx = 0;
    freed_next_idx = 0;
    allocated_next_idx = 0;
    allocation_error_emulation_flag = 0;
    allocation_error_emulation_after_nth_calls = -1;
}

void *mock_malloc(size_t size, char *file, int line)
{
    if (allocation_error_emulation_flag)
        return NULL;

    if (allocation_error_emulation_after_nth_calls && allocated_next_idx == allocation_error_emulation_after_nth_calls)
        return NULL;

    struct allocated *tr = &mem_allocated[allocated_next_idx++];
    uint8_t *p = mem + mem_next_idx;
    tr->underflow_guard = p;
    tr->mem = p + GUARD_SIZE;
    tr->overflow_guard = p + GUARD_SIZE + size;
    tr->file = file;
    tr->line = line;
    tr->size = size;
    memset(tr->underflow_guard, GUARD_BYTE, GUARD_SIZE);
    memset(tr->mem, 0xf0, size);
    memset(tr->overflow_guard, GUARD_BYTE, GUARD_SIZE);
    mem_next_idx += (GUARD_SIZE << 1) + size;
    return (void*)tr->mem;
}

void *mock_calloc(size_t nmemb, size_t size, char *file, int line)
{
    void *p = mock_malloc(nmemb * size, file, line);
    if (p)
        memset(p, 0, nmemb * size);
    return p;
}

void mock_free(void *ptr, char *file, int line)
{
    struct freed *tr = &mem_freed[freed_next_idx++];
    tr->mem = ptr;
    tr->file = file;
    tr->line = line;
}

#define malloc(size) mock_malloc(size, __FILE__, __LINE__)
#define calloc(nmemb, size) mock_calloc(nmemb, size, __FILE__, __LINE__)
#define free(p) mock_free(p, __FILE__, __LINE__)

#define memory_allocated      allocated_next_idx > 0
#define memory_not_allocated  allocated_next_idx == 0
#define memory_released       freed_next_idx > 0
#define memory_not_released   freed_next_idx == 0
#define allocated_count       allocated_next_idx
#define released_count        freed_next_idx

// ASSERTION & MESSAGE
struct allocated *check_memory_leaks()
{
    for(size_t i = 0; i < allocated_next_idx; i++)
    {
        int found = 0;
        for(size_t j = 0; j < freed_next_idx; j++)
        {
            if (mem_allocated[i].mem == mem_freed[j].mem)
            {
                found = 1;
                break;
            }
        }
        if (!found)
            return &mem_allocated[i];
    }
    return NULL;
}

int overflow_detect(uint8_t *buffer)
{
    uint8_t guard[GUARD_SIZE];
    memset(guard, GUARD_BYTE, GUARD_SIZE);
    if (0 != memcmp(buffer, guard, GUARD_SIZE))
        return 1;
    return 0;
}

struct allocated *check_buffer_underflow()
{
    for(size_t i = 0; i < allocated_next_idx; i++)
    {
        if (overflow_detect(mem_allocated[i].underflow_guard))
            return &mem_allocated[i];
    }
    return NULL;
}

struct allocated *check_buffer_overflow()
{
    for(size_t i = 0; i < allocated_next_idx; i++)
    {
        if (overflow_detect(mem_allocated[i].overflow_guard))
            return &mem_allocated[i];
    }
    return NULL;
}

#define MAX_MESSAGE_BUFFER 1024
static char message_buffer[MAX_MESSAGE_BUFFER];

void reset_message_buffer()
{
    memset(message_buffer, 0, MAX_MESSAGE_BUFFER);
}

void message(char *msg, char *file, int line)
{
    snprintf(message_buffer,
             MAX_MESSAGE_BUFFER,
             "%s:%d %s", file, line, msg);
}

void ext_message(char *msg, char *file, int line, char *real_file, int real_line)
{
    snprintf(message_buffer,
             MAX_MESSAGE_BUFFER,
             "%s:%d %s %s:%d", file, line, msg, real_file, real_line);
}

#define ASSERTION(cond, msg)              \
do{                                       \
    if (!(cond))                          \
    {                                     \
        message(msg, __FILE__, __LINE__); \
        return message_buffer;            \
    }                                     \
} while(0);                               \

#define DETECT_MEMORY_LEAK                                                    \
do{                                                                           \
    struct allocated *p = check_memory_leaks();                               \
    if (p != NULL)                                                            \
    {                                                                         \
        ext_message("Memory leak at", __FILE__, __LINE__, p->file, p->line);  \
        return message_buffer;                                                \
    }                                                                         \
} while(0);                                                                   \

#define DETECT_BUFFER_UNDERFLOW                                                                   \
do{                                                                                               \
    struct allocated *p = check_buffer_underflow();                                               \
    if (p != NULL)                                                                                \
    {                                                                                             \
        ext_message("Buffer underflow for mem allocated", __FILE__, __LINE__, p->file, p->line);  \
        return message_buffer;                                                                    \
    }                                                                                             \
} while(0);                                                                                       \

#define DETECT_BUFFER_OVERFLOW                                                                    \
do{                                                                                               \
    struct allocated *p = check_buffer_overflow();                                                \
    if (p != NULL)                                                                                \
    {                                                                                             \
        ext_message("Buffer overflow for mem allocated", __FILE__, __LINE__, p->file, p->line);   \
        return message_buffer;                                                                    \
    }                                                                                             \
} while(0);                                                                                       \


//  RUN TEST
typedef char* (*test_func_t)(void);
typedef void (*test_setup_func_t)(void);

struct test_case
{
    char *test_description;
    test_func_t test_func;
};

void run_one_test(struct test_case *test_case, test_setup_func_t setUpFunc)
{
    reset_mem();
    reset_message_buffer();

    if (setUpFunc)
        setUpFunc();

    char *msg = test_case->test_func();
    printf("%s: ", test_case->test_description);
    if (msg != NULL)
        printf(" FAIL!\n    %s\n", msg);
    else
        printf(" OK!\n");
}

void run_tests(struct test_case *test_cases, test_setup_func_t setUpFunc)
{
    size_t test_executed = 0;
    size_t test_ok = 0;
    size_t test_fail = 0;

    struct test_case *tc = test_cases;
    while (tc->test_func != NULL)
    {
        // setup
        reset_mem();
        reset_message_buffer();
        if (setUpFunc)
            setUpFunc();

        // run test
        char *msg = tc->test_func();
        printf("%-46s ", tc->test_description);
        if (msg != NULL)
        {
            printf(" FAIL!\n    %s\n", msg);
            test_fail++;
        }
        else
        {
            printf(" OK!\n");
            test_ok++;
        }
        test_executed++;
        tc++;
    }

    printf("-------------------------------------------------------------\n");
    printf("Test executed:     %32lu\n", test_executed);
    printf("Test completed ok: %32lu\n", test_ok);
    printf("Test failed:       %32lu\n\n", test_fail);
    if (test_fail)
        printf("!!!! TESTS HAVE NOT PASSED !!!!\n\n\n");
}

/// TEST FOR
#define _STDLIB_H  // DIRTY HACK - We prevent using stdlib.h (only malloc, calloc, free used in htable.c)
#include "htable.c"
#undef _STDLIB_H

/// TEST CASES
struct test_global_state {
    htable_t ht;
    htable_item_base_t *arr[4];
    htable_item_base_t *enumerated[4];
    size_t hash_func_calls_count;
    size_t destructor_calls_count;
    size_t enum_callback_calls_count;
} tgst;

uint32_t const_hash_func(__attribute__((unused)) const uint8_t *key, __attribute__((unused)) size_t key_len)
{
    tgst.hash_func_calls_count++;
    return 0;
}

uint32_t const_hash_func_2(__attribute__((unused)) const uint8_t *key, __attribute__((unused)) size_t key_len)
{
    tgst.hash_func_calls_count++;
    return 3;
}

bool items_equal(htable_item_base_t *item1, htable_item_base_t *item2)
{
    if (item1->key_len != item2->key_len)
        return false;
    int res = memcmp(item1->key, item2->key, item1->key_len);
    return !res;
}

bool items_array_equal(htable_t *ht, htable_item_base_t *items)
{
    int res = memcmp(ht->items, items, ht->capacity*sizeof(htable_item_base_t*));
    return !res;
}

bool htable_equal(htable_t *h1, htable_t *h2)
{
    return h1->last_error == h2->last_error &&
           h1->item_destructor == h2->item_destructor &&
           h1->hash_func == h2->hash_func &&
           h1->capacity == h2->capacity &&
           h1->items_count == h2->items_count &&
           h1->items == h2->items;
}

void mock_item_destructor(htable_item_base_t *item)
{
    tgst.destructor_calls_count++;
    default_item_destructor(item);
}

void set_up_test()
{
    tgst.hash_func_calls_count = 0;
    tgst.destructor_calls_count = 0;
    tgst.enum_callback_calls_count = 0;

    tgst.ht.capacity = 4;
    tgst.ht.items = tgst.arr;
    tgst.ht.items_count = 0;
    tgst.ht.item_destructor = mock_item_destructor;
    tgst.ht.hash_func = const_hash_func;
    tgst.ht.last_error = HTABLE_OK;
    memset(tgst.arr, 0, 4 * sizeof(htable_item_base_t*));
    memset(tgst.enumerated, 0, 4 * sizeof(htable_item_base_t*));
}

char *test_default_item_destructor()
{
    htable_item_base_t item = {(uint8_t*)"KEY", 3};
    default_item_destructor(&item);
    ASSERTION(memory_released, "Expected: Memory was released!")
    ASSERTION(mem_freed[0].mem == item.key, "Expected: Key was freed!")
    ASSERTION(mem_freed[1].mem == (uint8_t*)&item, "Expected: Item was freed!")
    return NULL;
}

char *test_compare_items_key_different_key_len()
{
    htable_item_base_t item1 = {(uint8_t*)"KEY", 3};
    htable_item_base_t item2 = {(uint8_t*)"PKEY", 4};
    bool res = compare_items_key(&item1, &item2);
    ASSERTION(res == false, "Expected: keys are not equal!")
    return NULL;
}

char *test_compare_items_key_different_key()
{
    htable_item_base_t item1 = {(uint8_t*)"FKEY", 4};
    htable_item_base_t item2 = {(uint8_t*)"PKEY", 4};
    bool res = compare_items_key(&item1, &item2);
    ASSERTION(res == false, "Expected: keys are not equal!")
    return NULL;
}

char *test_compare_items_key_the_same_key()
{
    htable_item_base_t item1 = {(uint8_t*)"PKEY", 4};
    htable_item_base_t item2 = {(uint8_t*)"PKEY", 4};
    bool res = compare_items_key(&item1, &item2);
    ASSERTION(res == true, "Expected: keys are equal!")
    return NULL;
}

char *test_expand_with_very_big_capacity()
{
    htable_t *ht = &tgst.ht;
    size_t big_capacity = ~0;
    tgst.ht.capacity = big_capacity;
    expand(ht);
    ASSERTION(ht->capacity == big_capacity, "Expected: Capacity was not changed!")
    ASSERTION(memory_not_allocated, "Expected: Memory was not allocated!")
    ASSERTION(memory_not_released, "Expected: Memory was not freed!")
    return NULL;
}

char *test_expand_mem_allocation_fail()
{
    htable_t *ht = &tgst.ht;
    allocation_error_emulation_flag = 1;
    expand(ht);
    ASSERTION(ht->capacity == 4, "Expected: Capacity was not changed!")
    ASSERTION(memory_not_allocated, "Expected: Memory was not allocated!")
    ASSERTION(memory_not_released, "Expected: Memory was not freed!")
    return NULL;
}

char *test_expand_empty_items()
{
    htable_t *ht = &tgst.ht;
    ht->items[0] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;

    expand(ht);

    ASSERTION(ht->capacity == 8, "Expected: capacity equals 8!")
    ASSERTION(allocated_count == 1, "Expected: Memory allocated one time!")
    ASSERTION(released_count == 1, "Expected: Memory released one time!")
    ASSERTION(mem_allocated[0].mem == (uint8_t *)ht->items, "Expected: Memory was allocated!")
    ASSERTION(mem_allocated[0].size == ht->capacity * sizeof(htable_item_base_t*), "Expected: Other size of memory was allocated!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)tgst.arr, "Expected: Memory was freed!")

    htable_item_base_t *p[8] = {0};
    ASSERTION(items_array_equal(ht, (htable_item_base_t*)p), "Expected: NO items in hash table!")
    ASSERTION(tgst.hash_func_calls_count == 0, "Expected: Hash function was not called!")
    return NULL;
}

char *test_expand()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = &item2;
    ht->items_count = 2;

    expand(ht);

    ASSERTION(ht->capacity == 8, "Expected: capacity equals 8!")
    ASSERTION(allocated_count == 1, "Expected: Memory allocated one time!")
    ASSERTION(released_count == 1, "Expected: Memory released one time!")
    ASSERTION(mem_allocated[0].mem == (uint8_t *)ht->items, "Expected: Memory was allocated!")
    ASSERTION(mem_allocated[0].size == ht->capacity * sizeof(htable_item_base_t*), "Expected: Other size of memory was allocated!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)tgst.arr, "Expected: Memory was released!")
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW

    htable_item_base_t *p[8] = {&item1, &item2, 0,0,0,0,0,0};
    ASSERTION(items_array_equal(ht, (htable_item_base_t*)p), "Expected: 2 first items.")
    ASSERTION(tgst.hash_func_calls_count == 2, "Expected: Hash function was called 2 times!")
    return NULL;
}

char *test_find_htable_full()
{
    htable_item_base_t item = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t for_search = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = marked_as_deleted;
    ht->items_count = 1;

    size_t out_idx = 0;
    bool res = find(ht, &for_search, &out_idx);

    ASSERTION(res == false, "Expected: Nothing was found!")
    ASSERTION(ht->last_error == HTABLE_FULL, "Expected: last_error is HTABLE_FULL!")
    ASSERTION(out_idx == 0, "Expected: out_index == 0!")
    ASSERTION(tgst.hash_func_calls_count == 1, "Expected: Hash function was called!")
    return NULL;
}

char *test_find_nothing_found_1()
{
    htable_item_base_t for_search = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    size_t out_idx = 0;
    bool res = find(ht, &for_search, &out_idx);

    ASSERTION(res == false, "Expected: Nothing was found!")
    ASSERTION(out_idx == 0, "Expected: out_index == 0!")
    ASSERTION(tgst.hash_func_calls_count == 1, "Expected: Hash function was called!")
    return NULL;
}

char *test_find_nothing_found_2()
{
    htable_item_base_t item = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t for_search = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item;
    ht->items_count = 1;

    size_t out_idx = 0;
    bool res = find(ht, &for_search, &out_idx);

    ASSERTION(res == false, "Expected: Nothing was found!")
    ASSERTION(out_idx == 0, "Expected: out_index == 0!") // index of first free or marked item
    ASSERTION(tgst.hash_func_calls_count == 1, "Expected: Hash function was called!")
    return NULL;
}

char *test_find_direct()
{
    htable_item_base_t item = {(uint8_t*)"ABC_KEY", 7};
    htable_item_base_t for_search = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item;
    ht->items_count = 1;

    size_t out_idx = 0;
    bool res = find(ht, &for_search, &out_idx);

    ASSERTION(res == true, "Expected: Item found successfully!")
    ASSERTION(out_idx == 0, "Expected: out_index == 0!")
    ASSERTION(tgst.hash_func_calls_count == 1, "Expected: Hash function was called!")
    return NULL;
}

char *test_find_relative_1()
{
    htable_item_base_t item = {(uint8_t*)"ABC_KEY", 7};
    htable_item_base_t for_search = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item;
    ht->items_count = 1;

    size_t out_idx = 0;
    bool res = find(ht, &for_search, &out_idx);

    ASSERTION(res == true, "Expected: Item was found successfully!")
    ASSERTION(out_idx == 2, "Expected: out_index == 2!")
    ASSERTION(tgst.hash_func_calls_count == 1, "Expected: Hash function was called!")
    return NULL;
}

char *test_find_relative_2()
{
    htable_item_base_t item1 = {(uint8_t*)"ABC_KEY", 7};
    htable_item_base_t item2 = {(uint8_t*)"FCC_KEY", 7};
    htable_item_base_t for_search = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item2;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item1;
    ht->items_count = 2;

    size_t out_idx = 0;
    bool res = find(ht, &for_search, &out_idx);

    ASSERTION(res == true, "Expected: Item was found successfully!")
    ASSERTION(out_idx == 2, "Expected: out_index == 2!")
    ASSERTION(tgst.hash_func_calls_count == 1, "Expected: Hash function was called!")
    return NULL;
}

char *test_htable_status()
{
    htable_status_t status;
    status = htable_status(NULL);
    ASSERTION(status == HTABLE_UNKNOWN, "Expected: Status HTABLE_UNKNOWN!")

    htable_t *ht = &tgst.ht;
    status = htable_status(ht);
    ASSERTION(status == HTABLE_OK, "Expected: Status HTABLE_OK!")
    return NULL;
}

char *test_htable_set_item_destructor()
{
    item_destructor_t d;
    d = htable_set_item_destructor(NULL, NULL);
    ASSERTION(d == NULL, "Expected: NULL!")

    htable_t *ht = &tgst.ht;
    d = htable_set_item_destructor(ht, NULL);
    ASSERTION(d == mock_item_destructor, "Expected: mock_item_destructor!")
    ASSERTION(ht->item_destructor == default_item_destructor, "Expected: default destructor!")

    d = htable_set_item_destructor(ht, mock_item_destructor);
    ASSERTION(d == default_item_destructor, "Expected: default destructor!")
    ASSERTION(ht->item_destructor == mock_item_destructor, "Expected: mock_item_destructor!")
    return  NULL;
}

char *test_htable_make()
{
    htable_t *ht;

    ht = htable_make(0, NULL);
    ASSERTION(ht != NULL, "Expected: not NULL!")
    ASSERTION(ht->capacity == 8, "Expected: capacity == 8!")
    ASSERTION(ht->items_count == 0, "Expected: items_count == 0!")
    ASSERTION(ht->last_error == HTABLE_OK, "Expected: HTABLE_OK!")
    ASSERTION(ht->hash_func == jenkins_one_at_a_time_hash, "Expected: default hash_function!")
    ASSERTION(ht->item_destructor == default_item_destructor, "Expected: default destructor!")
    ASSERTION(allocated_count == 2, "Expected: Memory allocated 1 times!")
    ASSERTION((uint8_t*)ht == mem_allocated[0].mem, "Expected: Memory for htable allocated!")
    ASSERTION(mem_allocated[0].size == sizeof(htable_t), "Expected: Allocated size equals size of htable_t!")
    ASSERTION((uint8_t*)ht->items == mem_allocated[1].mem, "Expected: Memory for items allocated!")
    ASSERTION(mem_allocated[1].size == 8*sizeof(htable_item_base_t*), "Expected: Allocated size equals 8 size of htable_item_base_t* !")
    return NULL;
}

char *test_htable_make_malloc_fail()
{
    htable_t *ht;
    allocation_error_emulation_flag = 1;
    ht = htable_make(0, NULL);
    ASSERTION(ht == NULL, "Expected: NULL")
    return NULL;
}

char *test_htable_make_with_specified_params()
{
    htable_t *ht;
    ht = htable_make(16, const_hash_func);
    ASSERTION(ht != NULL, "Expected: not NULL!")
    ASSERTION(ht->capacity == 16, "Expected: capacity == 16!")
    ASSERTION(ht->hash_func == const_hash_func, "Expected: Specified hash function!")
    return NULL;
}

char *test_htable_make_calloc_fail()
{
    htable_t *ht;
    allocation_error_emulation_after_nth_calls = 1;
    ht = htable_make(0, NULL);
    ASSERTION(ht == NULL, "Expected: NULL")
    DETECT_MEMORY_LEAK
    return NULL;
}

char *test_htable_set_wrong_parameters()
{
    htable_t exp_ht, *act_ht;
    act_ht = &tgst.ht;

    memcpy(&exp_ht, act_ht, sizeof(htable_t));

    htable_set(NULL, NULL, sizeof(htable_item_base_t));
    ASSERTION(htable_equal(&exp_ht, act_ht), "Expected: Hash table was not changed!")

    htable_set(act_ht, NULL, sizeof(htable_item_base_t));
    ASSERTION(htable_equal(&exp_ht, act_ht), "Expected: Hash table was not changed!")

    htable_item_base_t item1 = {(uint8_t*)NULL, 0};
    htable_set(act_ht, &item1, sizeof(htable_item_base_t));
    ASSERTION(htable_equal(&exp_ht, act_ht), "Expected: Hash table was not changed!")

    htable_item_base_t item2 = {(uint8_t*)"KEY", 3};
    htable_set(act_ht, &item2, 1);
    ASSERTION(htable_equal(&exp_ht, act_ht), "Expected: Hash table was not changed!")
    return NULL;
}

char *test_htable_set()
{
    htable_item_base_t item1 = {(uint8_t*)"KEY", 3};
    htable_t *ht = &tgst.ht;

    htable_set(ht, &item1, sizeof(htable_item_base_t));
    ASSERTION(allocated_count == 2, "Expected: Memory allocated 2 times!")
    ASSERTION(ht->items_count == 1, "Expected: Item was added to hash table")
    ASSERTION(mem_allocated[0].mem == (uint8_t *)ht->items[0], "Expected: Memory allocated!")
    ASSERTION(mem_allocated[0].size == sizeof(htable_item_base_t), "Expected: Memory allocated size is sizeof(htable_base_t)")
    ASSERTION(mem_allocated[1].mem == (uint8_t *)ht->items[0]->key, "Expected: Memory allocated for key!")
    ASSERTION(mem_allocated[1].size == 3, "Expected: Memory allocated size is 3!")
    ASSERTION(items_equal(ht->items[0], &item1), "Expected: Items are equal!")
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW
    return NULL;
}

char *test_htable_set_full()
{
    htable_item_base_t item1 = {(uint8_t*)"KEY", 3};
    htable_t exp_ht, *act_ht = &tgst.ht;

    act_ht->last_error = HTABLE_FULL;
    memcpy(&exp_ht, act_ht, sizeof(htable_t));

    htable_set(act_ht, &item1, sizeof(htable_item_base_t));
    ASSERTION(htable_equal(&exp_ht, act_ht), "Expected: Hash table was not changed!")
    ASSERTION(memory_not_allocated, "Expected: Memory was not allocated!")
    return NULL;
}

char *test_htable_set_mem_fail_1()
{
    htable_item_base_t item1 = {(uint8_t*)"KEY", 3};
    htable_t *ht = &tgst.ht;

    allocation_error_emulation_after_nth_calls = 1;
    htable_set(ht, &item1, sizeof(htable_item_base_t));

    ASSERTION(ht->last_error == HTABLE_MEM_ERROR, "Expected: Mem error status!")
    ASSERTION(ht->items_count == 0, "Expected: Item count not changed!")
    DETECT_MEMORY_LEAK
    return NULL;
}

char *test_htable_set_mem_fail_2()
{
    htable_item_base_t item1 = {(uint8_t*)"KEY", 3};
    htable_t *ht = &tgst.ht;

    allocation_error_emulation_flag = 1;
    htable_set(ht, &item1, sizeof(htable_item_base_t));

    ASSERTION(ht->last_error == HTABLE_MEM_ERROR, "Expected: Mem error status!")
    ASSERTION(ht->items_count == 0, "Expected: Item count not changed!")
    ASSERTION(memory_not_allocated, "Expected: Memory was not allocated!")
    return NULL;
}

char *test_htable_set_change_item()
{
    htable_item_base_t item1 = {(uint8_t*)"KEY", 3};
    htable_t *ht = &tgst.ht;
    ht->items[0] = &item1;
    ht->items_count = 1;

    htable_set(ht, &item1, sizeof(htable_item_base_t));
    ASSERTION(ht->items_count == 1, "Expected: Item count not changed!")
    ASSERTION(tgst.destructor_calls_count == 1, "Expected: Item destructor was called!")
    ASSERTION(allocated_count == 2, "Expected: Memory was allocated 2 times!")
    ASSERTION(released_count == 2, "Expected: Memory was released 2 times!")
    ASSERTION(mem_allocated[0].mem == (uint8_t*)ht->items[0], "Expected: New memory allocated for item!")
    ASSERTION(mem_allocated[0].size == sizeof(htable_item_base_t), "Expected: Size of New memory allocated!")
    ASSERTION(mem_allocated[1].mem == (uint8_t*)ht->items[0]->key, "Expected: New memory allocated for item key!")
    ASSERTION(mem_allocated[1].size == item1.key_len, "Expected: Size of New memory allocated for item key!")
    DETECT_BUFFER_OVERFLOW
    DETECT_BUFFER_UNDERFLOW
    return NULL;
}

char *test_htable_set_expand()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_item_base_t item3 = {(uint8_t*)"ABCD_KEY", 8};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = &item3;
    ht->items[3] = &item2;
    ht->items_count = 3;

    htable_item_base_t item4 = {(uint8_t*)"4PT_KEY", 7};
    htable_set(ht, &item4, sizeof(htable_item_base_t));
    ASSERTION(ht->capacity == 8, "Expected: Capacity == 8!")
    ASSERTION(items_equal(ht->items[0], &item1), "Expected: item1!")
    ASSERTION(items_equal(ht->items[1], &item3), "Expected: item3!")
    ASSERTION(items_equal(ht->items[2], &item2), "Expected: item2!")
    ASSERTION(items_equal(ht->items[3], &item4), "Expected: item4!")
    ASSERTION(ht->items[4] == NULL, "Expected: NULL!")
    return NULL;
}

char *test_htable_destroy_wrong_param()
{
    htable_t exp_ht, *act_ht;
    act_ht = &tgst.ht;

    memcpy(&exp_ht, act_ht, sizeof(htable_t));

    htable_destroy(NULL);
    ASSERTION(memory_not_released, "Expected: Memory was not freed!")
    ASSERTION(htable_equal(&exp_ht, act_ht), "Expected: Hash table was not changed!")
    return NULL;
}

char *test_htable_destroy()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_item_base_t item3 = {(uint8_t*)"ABCD_KEY", 8};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = &item3;
    ht->items[3] = &item2;
    ht->items_count = 3;

    htable_destroy(ht);
    ASSERTION(tgst.destructor_calls_count == 3, "Expected: Destructor was called 3 times!")
    ASSERTION(released_count == 8, "Expected: Memory was released 8 times!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)item1.key, "Expected: item1 key released!")
    ASSERTION(mem_freed[1].mem == (uint8_t*)&item1, "Expected: item1 released!")
    ASSERTION(mem_freed[2].mem == (uint8_t*)item3.key, "Expected: item3 key released!")
    ASSERTION(mem_freed[3].mem == (uint8_t*)&item3, "Expected: item3 released!")
    ASSERTION(mem_freed[4].mem == (uint8_t*)item2.key, "Expected: item2 key released!")
    ASSERTION(mem_freed[5].mem == (uint8_t*)&item2, "Expected: item2 released!")
    ASSERTION(mem_freed[6].mem == (uint8_t*)ht->items, "Expected: items released!")
    ASSERTION(mem_freed[7].mem == (uint8_t*)ht, "Expected: htable released!")
    return NULL;
}

int enum_callback(htable_item_base_t *item)
{
    tgst.enumerated[tgst.enum_callback_calls_count++] = item;
    return 1;
}

int enum_callback_stop(htable_item_base_t *item)
{
    tgst.enumerated[tgst.enum_callback_calls_count++] = item;
    return 0;
}

char *test_htable_enumerate_items_wrong_params()
{
    htable_enumerate_items(NULL, enum_callback);
    ASSERTION(tgst.enum_callback_calls_count == 0, "Expected: Callback was not called!")
    htable_enumerate_items(&tgst.ht, NULL);
    ASSERTION(tgst.enum_callback_calls_count == 0, "Expected: Callback was not called!")
    return NULL;
}

char *test_htable_enumerate_items()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = &item2;
    ht->items_count = 2;

    htable_enumerate_items(ht, enum_callback);
    ASSERTION(tgst.enum_callback_calls_count == 2, "Expected: Callback was called 2 times!")
    ASSERTION(tgst.enumerated[0] == &item1, "Expected: item1 was enumerated!")
    ASSERTION(tgst.enumerated[1] == &item2, "Expected: item2 was enumerated!")
    return NULL;
}

char *test_htable_enumerate_items_stop()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = &item2;
    ht->items_count = 2;

    htable_enumerate_items(ht, enum_callback_stop);
    ASSERTION(tgst.enum_callback_calls_count == 1, "Expected: Callback was called 1 times!")
    ASSERTION(tgst.enumerated[0] == &item1, "Expected: item1 was enumerated!")
    return NULL;
}

char *test_htable_find_wrong_params()
{
    htable_t *ht = &tgst.ht;
    htable_item_base_t item = {NULL, 0};
    bool res;

    res = htable_find(NULL, NULL, NULL);
    ASSERTION(res == false, "Expected: Nothing was found!")
    res = htable_find(ht, NULL, NULL);
    ASSERTION(res == false, "Expected: Nothing was found!")
    res = htable_find(ht, &item, NULL);
    ASSERTION(res == false, "Expected: Nothing was found!")
    return NULL;
}

char *test_htable_find()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = &item2;
    ht->items_count = 2;

    htable_item_base_t *out = NULL;
    bool res = htable_find(ht, &item2, &out);
    ASSERTION(res == true, "Expected: Item was found!")
    ASSERTION(out != NULL, "Expected: Out parameter is not NULL!")
    ASSERTION(items_equal(out, &item2), "Expected: Out parameter is item2!")
    return NULL;
}

char *test_htable_find_no_out_item()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = &item1;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = &item2;
    ht->items_count = 2;

    bool res = htable_find(ht, &item2, NULL);
    ASSERTION(res == true, "Expected: Item was found!")
    return NULL;
}

char *test_htable_find_not_found()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item1;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item2;
    ht->items_count = 2;

    htable_item_base_t item3 = {(uint8_t*)"HHH", 3};
    bool res = htable_find(ht, &item3, NULL);
    ASSERTION(res == false, "Expected: Item was not found!")
    return NULL;
}

char *test_htable_remove_wrong_params()
{
    htable_t *ht = &tgst.ht;
    htable_item_base_t item = {NULL, 0};
    bool res;

    res = htable_remove(NULL, NULL);
    ASSERTION(res == false, "Expected: Nothing was removed!")
    res = htable_remove(ht, NULL);
    ASSERTION(res == false, "Expected: Nothing was removed!")
    res = htable_remove(ht, &item);
    ASSERTION(res == false, "Expected: Nothing was removed!")
    return NULL;
}

char *test_htable_remove_exist_key()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item1;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item2;
    ht->items_count = 2;

    bool res = htable_remove(ht, &item2);
    ASSERTION(res == true, "Expected: Item was found!")
    ASSERTION(ht->items[2] == marked_as_deleted, "Expected: Item was removed!")
    ASSERTION(tgst.destructor_calls_count == 1, "Expected: Item destructor was called 1 time!")
    ASSERTION(released_count == 2, "Expected: Memory was released 2 times!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)item2.key, "Expected: item1 key was released!")
    ASSERTION(mem_freed[1].mem == (uint8_t*)&item2, "Expected: item1 was released!")
    ASSERTION(ht->items_count == 1, "Expected: items count == 1!")
    ASSERTION(ht->capacity == 4, "Expected: capacity == 4!")
    return NULL;
}

char *test_htable_remove_not_exist_key()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item1;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item2;
    ht->items_count = 2;

    htable_item_base_t item3 = {(uint8_t*)"HHH", 3};
    bool res = htable_remove(ht, &item3);
    ASSERTION(res == false, "Expected: Item was not found!")
    ASSERTION(tgst.destructor_calls_count == 0, "Expected: Item destructor was not called!")
    ASSERTION(released_count == 0, "Expected: Memory was not released!")
    ASSERTION(ht->items_count == 2, "Expected: items count == 2!")
    ASSERTION(ht->capacity == 4, "Expected: capacity == 4!")
    return NULL;
}

char *test_htable_pop_wrong_params()
{
    htable_t *ht = &tgst.ht;
    htable_item_base_t item = {NULL, 0};
    htable_item_base_t *res;

    res = htable_pop(NULL, NULL);
    ASSERTION(res == NULL, "Expected: Nothing was removed!")
    res = htable_pop(ht, NULL);
    ASSERTION(res == NULL, "Expected: Nothing was removed!")
    res = htable_pop(ht, &item);
    ASSERTION(res == NULL, "Expected: Nothing was removed!")
    return NULL;
}

char *test_htable_pop_exist_key()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item1;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item2;
    ht->items_count = 2;

    htable_item_base_t *res = htable_pop(ht, &item2);
    ASSERTION(res == &item2, "Expected: Item was found!")
    ASSERTION(ht->items[2] == marked_as_deleted, "Expected: Item was removed!")
    ASSERTION(tgst.destructor_calls_count == 0, "Expected: Item destructor was not called!")
    ASSERTION(memory_not_released, "Expected: Memory was not released!")
    ASSERTION(ht->items_count == 1, "Expected: items count == 1!")
    ASSERTION(ht->capacity == 4, "Expected: capacity == 4!")
    return NULL;
}

char *test_htable_pop_not_exist_key()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->items[0] = &item1;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = &item2;
    ht->items_count = 2;

    htable_item_base_t item3 = {(uint8_t*)"HHH", 3};
    htable_item_base_t *res = htable_pop(ht, &item3);
    ASSERTION(res == NULL, "Expected: Item was not found!")
    ASSERTION(ht->items_count == 2, "Expected: items count == 2!")
    ASSERTION(ht->capacity == 4, "Expected: capacity == 4!")
    return NULL;
}

char *test_functional()
{
    htable_t *ht =  htable_make(8, NULL);
    ASSERTION(ht != NULL, "Expected: Hash table was created!")

    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"AT_KEY_2", 8};
    htable_item_base_t item3 = {(uint8_t*)"CC_KEY_22", 9};
    htable_item_base_t item4 = {(uint8_t*)"PU_KEY_333", 10};
    htable_item_base_t item5 = {(uint8_t*)"HASH TABLE DONT CONTAIN THIS KEY", 32};

    htable_set(ht, &item1, sizeof(htable_item_base_t));
    htable_set(ht, &item2, sizeof(htable_item_base_t));
    htable_set(ht, &item3, sizeof(htable_item_base_t));
    htable_set(ht, &item4, sizeof(htable_item_base_t));
    ASSERTION(ht->items_count == 4, "Expected: items count == 4!")
    ASSERTION(ht->capacity == 8, "Expected: capacity == 8!")

    htable_item_base_t *out = NULL;
    bool res;

    res = htable_find(ht, &item2, NULL);
    ASSERTION(res == true, "Expected: Item was found!")
    res = htable_find(ht, &item3, NULL);
    ASSERTION(res == true, "Expected: Item was found!")
    res = htable_find(ht, &item4, NULL);
    ASSERTION(res == true, "Expected: Item was found!")
    res = htable_find(ht, &item5, NULL);
    ASSERTION(res == false, "Expected: Item was not found!")

    res = htable_find(ht, &item1, &out);
    ASSERTION(res == true, "Expected: Item was found!")
    ASSERTION(out != NULL, "Expected: Out parameter is not NULL!")
    ASSERTION(items_equal(out, &item1), "Expected: Key matched!")

    res = htable_remove(ht, &item4);
    ASSERTION(res == true, "Expected: Item was found!")
    res = htable_remove(ht, &item5);
    ASSERTION(res == false, "Expected: Item was not found!")

    out = htable_pop(ht, &item5);
    ASSERTION(out == NULL, "Expected: Item was not found!")
    out = htable_pop(ht, &item1);
    ASSERTION(out != NULL, "Expected: Item was found!")
    default_item_destructor(out);

    htable_destroy(ht);
    DETECT_MEMORY_LEAK
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW
    return NULL;
}

char *test_htable_set_last_item() {
    htable_item_base_t item1 = {(uint8_t *) "PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t *) "ABCD_KEY", 8};
    htable_t *ht = &tgst.ht;

    ht->hash_func = const_hash_func_2;
    ht->items[3] = &item2;
    ht->items_count = 1;

    htable_set(ht, &item1, sizeof(htable_item_base_t));
    ASSERTION(ht->items_count == 2, "Expected: items count == 2!")
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW

    ASSERTION(ht->items[0] != NULL, "Expected: Item was added!")
    ASSERTION(items_equal(ht->items[0], &item1), "Expected: Keys matched!")
    return NULL;
}

char *test_htable_find_last_item()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->hash_func = const_hash_func_2;
    ht->items[0] = &item1;
    ht->items[3] = &item2;
    ht->items_count = 2;

    htable_item_base_t *out = NULL;
    bool res = htable_find(ht, &item1, &out);
    ASSERTION(res == true, "Expected: Item was found!")
    ASSERTION(out != NULL, "Expected: Out parameter is not NULL!")
    ASSERTION(items_equal(out, &item1), "Expected: Out parameter is item2!")
    return NULL;
}

char *test_htable_remove_exist_key_last_item()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->hash_func = const_hash_func_2;
    ht->items[0] = &item1;
    ht->items[3] = &item2;
    ht->items_count = 2;

    bool res = htable_remove(ht, &item1);
    ASSERTION(res == true, "Expected: Item was found!")
    ASSERTION(ht->items[0] == marked_as_deleted, "Expected: Item was removed!")
    ASSERTION(tgst.destructor_calls_count == 1, "Expected: Item destructor was called 1 time!")
    ASSERTION(released_count == 2, "Expected: Memory was released 2 times!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)item1.key, "Expected: item1 key was released!")
    ASSERTION(mem_freed[1].mem == (uint8_t*)&item1, "Expected: item1 was released!")
    ASSERTION(ht->items_count == 1, "Expected: items count == 1!")
    ASSERTION(ht->capacity == 4, "Expected: capacity == 4!")
    return NULL;
}

char *test_htable_pop_exist_key_last_item()
{
    htable_item_base_t item1 = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t item2 = {(uint8_t*)"ABC_KEY", 7};
    htable_t *ht = &tgst.ht;

    ht->hash_func = const_hash_func_2;
    ht->items[0] = &item1;
    ht->items[3] = &item2;
    ht->items_count = 2;

    htable_item_base_t *res = htable_pop(ht, &item1);
    ASSERTION(res == &item1, "Expected: Item was found!")
    ASSERTION(ht->items[0] == marked_as_deleted, "Expected: Item was removed!")
    ASSERTION(tgst.destructor_calls_count == 0, "Expected: Item destructor was not called!")
    ASSERTION(memory_not_released, "Expected: Memory was not released!")
    ASSERTION(ht->items_count == 1, "Expected: items count == 1!")
    ASSERTION(ht->capacity == 4, "Expected: capacity == 4!")
    return NULL;
}

char *test_htable_set_special_case() {
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = marked_as_deleted;

    htable_item_base_t item = {(uint8_t*)"PT_KEY", 6};
    htable_set(ht, &item, sizeof(htable_item_base_t));
    ASSERTION(ht->last_error != HTABLE_FULL, "Expected: Htable status is not HTABLE FULL!")
    ASSERTION(ht->items_count == 1, "Expected: items count == 1!")
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW

    ASSERTION(ht->items[0] != NULL, "Expected: Item was added!")
    ASSERTION(items_equal(ht->items[0], &item), "Expected: Keys matched!")
    return NULL;
}

char *test_htable_find_special_case() {
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = marked_as_deleted;

    htable_item_base_t item = {(uint8_t*)"PT_KEY", 6};
    bool res = htable_find(ht, &item, NULL);
    ASSERTION(ht->last_error != HTABLE_FULL, "Expected: Htable status is not HTABLE FULL!")
    ASSERTION(res == false, "Expected: Nothing was found!")
    return NULL;
}

char *test_htable_remove_special_case() {
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = marked_as_deleted;

    htable_item_base_t item = {(uint8_t*)"PT_KEY", 6};
    bool res = htable_remove(ht, &item);
    ASSERTION(ht->last_error != HTABLE_FULL, "Expected: Htable status is not HTABLE FULL!")
    ASSERTION(res == false, "Expected: Nothing was found!")
    return NULL;
}

char *test_htable_pop_special_case() {
    htable_t *ht = &tgst.ht;

    ht->items[0] = marked_as_deleted;
    ht->items[1] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;
    ht->items[3] = marked_as_deleted;

    htable_item_base_t item = {(uint8_t*)"PT_KEY", 6};
    htable_item_base_t *res = htable_pop(ht, &item);
    ASSERTION(ht->last_error != HTABLE_FULL, "Expected: Htable status is not HTABLE FULL!")
    ASSERTION(res == NULL, "Expected: Nothing was found!")
    return NULL;
}

/// ----------------------------------------------------------------
int main(void)
{
    struct test_case tests[] = {
        {"Test default_item_destructor", test_default_item_destructor},
        {"Test compare_items_key 1", test_compare_items_key_different_key_len},
        {"Test compare_items_key 2", test_compare_items_key_different_key},
        {"Test compare_items_key 3", test_compare_items_key_the_same_key},
        {"Test expand 1", test_expand_with_very_big_capacity},
        {"Test expand 2", test_expand_mem_allocation_fail},
        {"Test expand 3", test_expand_empty_items},
        {"Test expand 4", test_expand},
        {"Test find 1", test_find_htable_full},
        {"Test find 2", test_find_nothing_found_1},
        {"Test find 3", test_find_nothing_found_2},
        {"Test find 4", test_find_direct},
        {"Test find 5", test_find_relative_1},
        {"Test find 6", test_find_relative_2},
        {"Test htable_status", test_htable_status},
        {"Test htable_set_item_destructor", test_htable_set_item_destructor},
        {"Test htable_make", test_htable_make},
        {"Test htable_make malloc fail", test_htable_make_malloc_fail},
        {"Test htable_make set hash function", test_htable_make_with_specified_params},
        {"Test htable_make calloc fail", test_htable_make_calloc_fail},
        {"Test htable_set wrong parameters", test_htable_set_wrong_parameters},
        {"Test htable_set ", test_htable_set},
        {"Test htable_set full ", test_htable_set_full},
        {"Test htable_set memory fail 1", test_htable_set_mem_fail_1},
        {"Test htable_set memory fail 2", test_htable_set_mem_fail_2},
        {"Test htable_set change item", test_htable_set_change_item},
        {"Test htable_set expand", test_htable_set_expand},
        {"Test htable_destroy wrong parameters", test_htable_destroy_wrong_param},
        {"Test htable_destroy", test_htable_destroy},
        {"Test htable_enumerate_items wrong params", test_htable_enumerate_items_wrong_params},
        {"Test htable_enumerate_items", test_htable_enumerate_items},
        {"Test htable_enumerate_items stop", test_htable_enumerate_items_stop},
        {"Test htable_find wrong params", test_htable_find_wrong_params},
        {"Test htable_find ", test_htable_find},
        {"Test htable_find no out_item ", test_htable_find_no_out_item},
        {"Test htable_find not found item ", test_htable_find_not_found},
        {"Test htable_remove wrong params ", test_htable_remove_wrong_params},
        {"Test htable_remove exist key ", test_htable_remove_exist_key},
        {"Test htable_remove not exist key ", test_htable_remove_not_exist_key},
        {"Test htable_pop wrong params ", test_htable_pop_wrong_params},
        {"Test htable_pop exist key ", test_htable_pop_exist_key},
        {"Test htable_pop not exist key ", test_htable_pop_not_exist_key},
        {"Test functional of hash table ", test_functional},
        {"Test htable_set last occupied item ", test_htable_set_last_item},
        {"Test htable_find last occupied item ", test_htable_find_last_item},
        {"Test htable_remove last occupied item ", test_htable_remove_exist_key_last_item},
        {"Test htable_pop last occupied item ", test_htable_pop_exist_key_last_item},
        {"Test htable_set special case ", test_htable_set_special_case},
        {"Test htable_find special case ", test_htable_find_special_case},
        {"Test htable_remove special case ", test_htable_remove_special_case},
        {"Test htable_pop special case ", test_htable_pop_special_case},
        {0, 0},
    };

    run_tests(tests, set_up_test);
    return 0;
}