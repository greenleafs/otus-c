#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

void reset_mem()
{
    memset(mem, 0, MEM_SIZE);
    memset(mem_freed, 0, MEMOP_TRACK_SIZE * sizeof(struct freed));
    memset(mem_allocated, 0, MEMOP_TRACK_SIZE * sizeof(struct allocated));
    mem_next_idx = 0;
    freed_next_idx = 0;
    allocated_next_idx = 0;
    allocation_error_emulation_flag = 0;
}

void *mock_malloc(size_t size, char *file, int line)
{
    if (allocation_error_emulation_flag)
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
        printf("%-36s ", tc->test_description);
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
    printf("Test executed:     %20lu\n", test_executed);
    printf("Test completed ok: %20lu\n", test_ok);
    printf("Test failed:       %20lu\n\n", test_fail);
    if (test_fail)
        printf("!!!! TESTS HAS NOT PASSED !!!!\n\n\n");
}

/// TEST FOR
#define _STDLIB_H  // DIRTY HACK - We prevent using stdlib.h (only malloc, calloc, free used in htable.c)
#include "htable.c"
#undef _STDLIB_H

/// TEST CASES
struct test_global_state {
    htable_t ht;
    htable_item_base_t *arr[4];
    size_t const_hash_func_calls_count;
    size_t destructor_calls_count;
} tgst;

uint32_t const_hash_func(__attribute__((unused)) const uint8_t *key, __attribute__((unused)) size_t key_len)
{
    tgst.const_hash_func_calls_count++;
    return 0;
}

void mock_item_destructor(htable_item_base_t *item)
{
    tgst.destructor_calls_count++;
    default_item_destructor(item);
}

void set_up_test()
{
    tgst.const_hash_func_calls_count = 0;
    tgst.destructor_calls_count = 0;

    tgst.ht.capacity = 4;
    tgst.ht.items = tgst.arr;
    tgst.ht.items_count = 0;
    tgst.ht.item_destructor = mock_item_destructor;
    tgst.ht.hash_func = const_hash_func;
    tgst.ht.last_error = HTABLE_OK;
    memset(tgst.arr, 0, 4 * sizeof(htable_item_base_t*));
}

char *test_default_item_destructor()
{
    htable_item_base_t item = {(uint8_t*)"KEY", 3};
    default_item_destructor(&item);
    ASSERTION(mem_freed[0].mem == item.key, "Key was not freed!")
    ASSERTION(mem_freed[1].mem == (uint8_t*)&item, "Item was not be freed!")
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
    ASSERTION(allocated_next_idx == 0, "Expected: Memory was not allocated!")
    ASSERTION(freed_next_idx == 0, "Expected: Memory was not freed!")
    return NULL;
}

char *test_expand_mem_allocation_fail()
{
    htable_t *ht = &tgst.ht;
    allocation_error_emulation_flag = 1;
    expand(ht);
    ASSERTION(ht->capacity == 4, "Expected: Capacity was not changed!")
    ASSERTION(allocated_next_idx == 0, "Expected: Memory was not allocated!")
    ASSERTION(freed_next_idx == 0, "Expected: Memory was not freed!")
    return NULL;
}

char *test_expand_empty_items()
{
    htable_t *ht = &tgst.ht;
    ht->items[0] = marked_as_deleted;
    ht->items[2] = marked_as_deleted;

    expand(ht);

    ASSERTION(ht->hash_func == const_hash_func, "Expected: Hash function was not changed")
    ASSERTION(tgst.const_hash_func_calls_count == 0, "Expected: Hash function was not called!")
    ASSERTION(ht->item_destructor == mock_item_destructor, "Expected: Destructor function was not changed")
    ASSERTION(tgst.destructor_calls_count == 0, "Expected: Destructor function was not called!")
    ASSERTION(ht->items_count == 0, "Expected: items_count equals 0")
    ASSERTION(ht->capacity == 8, "Expected: capacity equals 8!")
    ASSERTION(mem_allocated[0].mem == (uint8_t *)ht->items, "Expected: Memory was allocated!")
    ASSERTION(mem_allocated[0].size == ht->capacity * sizeof(htable_item_base_t*), "Expected: Other size of memory was allocated!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)tgst.arr, "Expected: Memory was freed!")
    ASSERTION(mem_allocated[1].mem == NULL, "Expected: No more memory was allocated!")
    ASSERTION(mem_freed[1].mem == NULL, "Expected: No more Memory was freed!")
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW

    htable_item_base_t *p[8] = {0};
    int res = memcmp(ht->items, p, ht->capacity * sizeof(htable_item_base_t*));
    ASSERTION(res == 0, "Expected: NO items in hash table!")
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
    ht->items[2] = &item2;
    ht->items_count = 2;

    expand(ht);

    ASSERTION(ht->hash_func == const_hash_func, "Expected: Hash function was not changed")
    ASSERTION(tgst.const_hash_func_calls_count == 2, "Expected: Hash function was called!")
    ASSERTION(ht->item_destructor == mock_item_destructor, "Expected: Destructor function was not changed")
    ASSERTION(tgst.destructor_calls_count == 0, "Expected: Destructor function was not called!")
    ASSERTION(ht->items_count == 2, "Expected: items_count equals 0")
    ASSERTION(ht->capacity == 8, "Expected: capacity equals 8!")
    ASSERTION(mem_allocated[0].mem == (uint8_t *)ht->items, "Expected: Memory was allocated!")
    ASSERTION(mem_allocated[0].size == ht->capacity * sizeof(htable_item_base_t*), "Expected: Other size of memory was allocated!")
    ASSERTION(mem_freed[0].mem == (uint8_t*)tgst.arr, "Expected: Memory was freed!")
    ASSERTION(mem_allocated[1].mem == NULL, "Expected: No more memory was allocated!")
    ASSERTION(mem_freed[1].mem == NULL, "Expected: No more Memory was freed!")
    DETECT_BUFFER_UNDERFLOW
    DETECT_BUFFER_OVERFLOW

    htable_item_base_t *p[8] = {&item1, &item2, 0,0,0,0,0,0};
    int res = memcmp(ht->items, p, ht->capacity * sizeof(htable_item_base_t*));
    ASSERTION(res == 0, "Expected: 2 first items.")

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
        {0, 0},
    };

    run_tests(tests, set_up_test);
    return 0;
}