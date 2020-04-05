#include <stddef.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "CuTest.h"
#include "../../booltype.h"
#define MEM_C_
#include "../../mem.h"
#undef MEM_C_


typedef struct 
{
    int val1;
    int val2;
    int val3;
} Test1;

typedef struct
{
    char vals[256];
} Test2;

/* wrap structs */
struct Test1_wrap
{
    struct arc_obj aoh;
    Test1 wrapped;
    struct arc_obj aot;
};
struct Test2_wrap
{
    struct arc_obj aoh;
    Test2 wrapped;
    struct arc_obj aot;
};

/* arc_obj_type definitions */
static struct arc_obj_type Test1_type = 
{
    .ao_count = 0,
    .name = "Test1",
    .aoh_offset = offsetof(struct Test1_wrap, aoh),
    .wrapped_offset = offsetof(struct Test1_wrap, wrapped),
    .aot_offset = offsetof(struct Test1_wrap, aot),
};
static struct arc_obj_type Test2_type = 
{
    .ao_count = 0,
    .name = "Test2",
    .aoh_offset = offsetof(struct Test2_wrap, aoh),
    .wrapped_offset = offsetof(struct Test2_wrap, wrapped),
    .aot_offset = offsetof(struct Test2_wrap, aot),
};

static struct arc_obj_type *all_arc_obj_types[] =
{
    &Test1_type,
    &Test2_type,
    NULL
};

/* lao offset definitions */

static struct Test1_wrap *Test1_get_wrap(Test1 *p)
{
    struct Test1_wrap *wr = (struct Test1_wrap *)((uintptr_t)p - offsetof(struct Test1_wrap, wrapped));
    return wr;
}

static struct Test2_wrap *Test2_get_wrap(Test2 *p)
{
    struct Test2_wrap *wr = (struct Test2_wrap *)((uintptr_t)p - offsetof(struct Test2_wrap, wrapped));
    return wr;
}


/* alloc and dealloc definitions */
Test1 *alloc_Test1(void)
{
    struct Test1_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&Test1_type, &wr->aoh, &wr->aot);
    return &wr->wrapped;
}

void dealloc_Test1(Test1 *p)
{
    struct Test1_wrap *wr = Test1_get_wrap(p);
    arc_obj_deinit(&Test1_type, &wr->aoh, &wr->aot);
    free(wr);
}

Test2 *alloc_Test2(void)
{
    struct Test2_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&Test2_type, &wr->aoh, &wr->aot);
    return &wr->wrapped;
}

void dealloc_Test2(Test2 *p)
{
    struct Test2_wrap *wr = Test2_get_wrap(p);
    arc_obj_deinit(&Test2_type, &wr->aoh, &wr->aot);
    free(wr);
}

static unsigned count_ao_list(const struct arc_obj *list)
{
    unsigned count = 0;
    for (const struct arc_obj *ao = list->ao_next; ao != list; ao = ao->ao_next)
    {
        ++count;
    }
    return count;
}

static bool ao_in_list(const struct arc_obj *list, struct arc_obj *ao_tgt)
{
    for (const struct arc_obj *ao = list->ao_next; ao != list; ao = ao->ao_next)
    {
        if (ao_tgt == ao)
            return TRUE;
    }
    return FALSE;
}

void Test_arc_obj(CuTest *tc)
{
    {
        // Single obj init and deinit
        CuAssertTrue(tc, 0 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 0 == count_ao_list(&all_aot));
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        
        Test1 *ptr = alloc_Test1();

        CuAssertTrue(tc, 1 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 1 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test1_get_wrap(ptr)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test1_get_wrap(ptr)->aot));
        CuAssertTrue(tc, 1 == Test1_type.ao_count);


        dealloc_Test1(ptr);

        CuAssertTrue(tc, 0 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 0 == count_ao_list(&all_aot));
        CuAssertTrue(tc, !ao_in_list(&all_aoh, &Test1_get_wrap(ptr)->aoh));
        CuAssertTrue(tc, !ao_in_list(&all_aot, &Test1_get_wrap(ptr)->aot));
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
    }

    {
        // Multiple obj init and deinit
        CuAssertTrue(tc, 0 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 0 == count_ao_list(&all_aot));
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        CuAssertTrue(tc, 0 == Test2_type.ao_count);

        Test1 *ptr1 = alloc_Test1();

        CuAssertTrue(tc, 1 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 1 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test1_get_wrap(ptr1)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test1_get_wrap(ptr1)->aot));
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 0 == Test2_type.ao_count);

        Test2 *ptr2 = alloc_Test2();

        CuAssertTrue(tc, 2 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 2 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test1_get_wrap(ptr1)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test1_get_wrap(ptr1)->aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test2_get_wrap(ptr2)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test2_get_wrap(ptr2)->aot));
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        Test2 *ptr3 = alloc_Test2();

        CuAssertTrue(tc, 3 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 3 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test1_get_wrap(ptr1)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test1_get_wrap(ptr1)->aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test2_get_wrap(ptr2)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test2_get_wrap(ptr2)->aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test2_get_wrap(ptr3)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test2_get_wrap(ptr3)->aot));
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 2 == Test2_type.ao_count);

        dealloc_Test2(ptr2);

        CuAssertTrue(tc, 2 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 2 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test1_get_wrap(ptr1)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test1_get_wrap(ptr1)->aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test2_get_wrap(ptr3)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test2_get_wrap(ptr3)->aot));
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        dealloc_Test1(ptr1);
        
        CuAssertTrue(tc, 1 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 1 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test2_get_wrap(ptr3)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test2_get_wrap(ptr3)->aot));
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);
                
        Test1 *ptr4 = alloc_Test1();

        CuAssertTrue(tc, 2 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 2 == count_ao_list(&all_aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test2_get_wrap(ptr3)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test2_get_wrap(ptr3)->aot));
        CuAssertTrue(tc, ao_in_list(&all_aoh, &Test1_get_wrap(ptr4)->aoh));
        CuAssertTrue(tc, ao_in_list(&all_aot, &Test1_get_wrap(ptr4)->aot));
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        dealloc_Test2(ptr3);
        dealloc_Test1(ptr4);

        CuAssertTrue(tc, 0 == count_ao_list(&all_aoh));
        CuAssertTrue(tc, 0 == count_ao_list(&all_aot));
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        CuAssertTrue(tc, 0 == Test2_type.ao_count);
    }
}