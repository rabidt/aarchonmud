#include <stddef.h>
#include <time.h>
#include <string.h>
#include "CuTest.h"
#include "../../merc.h"


struct Test1
{
    ARC_OBJ_HEAD
};
static struct arc_obj_type Test1_type = { .name = "Test1" };

struct Test2
{
    ARC_OBJ_HEAD
};
static struct arc_obj_type Test2_type = { .name = "Test2" };


static unsigned count_all_arc_obj(void)
{
    unsigned count = 0;
    for (const struct arc_obj *ao = all_arc_obj.ao_next; ao != &all_arc_obj; ao = ao->ao_next)
    {
        ++count;
    }
    return count;
}

void Test_arc_obj(CuTest *tc)
{
    {
        // Single obj init and deinit
        struct Test1 *ptr = malloc(sizeof(*ptr));

        CuAssertTrue(tc, !AO_VALID(ptr));
        CuAssertTrue(tc, 0 == count_all_arc_obj());
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        
        arc_obj_init(&ptr->ao_, &Test1_type);

        CuAssertTrue(tc, AO_VALID(ptr));
        CuAssertTrue(tc, 1 == count_all_arc_obj());
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        
        arc_obj_deinit(&ptr->ao_);

        CuAssertTrue(tc, !AO_VALID(ptr));
        CuAssertTrue(tc, 0 == count_all_arc_obj());
        CuAssertTrue(tc, 0 == Test1_type.ao_count);

        free(ptr);
    }

    {
        // Multiple obj init and deinit
        struct Test1 *ptr1 = malloc(sizeof(*ptr1));
        struct Test2 *ptr2 = malloc(sizeof(*ptr2));
        struct Test2 *ptr3 = malloc(sizeof(*ptr3));
        struct Test1 *ptr4 = malloc(sizeof(*ptr4));

        CuAssertTrue(tc, !AO_VALID(ptr1));
        CuAssertTrue(tc, !AO_VALID(ptr2));
        CuAssertTrue(tc, !AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 0 == count_all_arc_obj());
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        CuAssertTrue(tc, 0 == Test2_type.ao_count);
        
        arc_obj_init(&ptr1->ao_, &Test1_type);

        CuAssertTrue(tc,  AO_VALID(ptr1));
        CuAssertTrue(tc, !AO_VALID(ptr2));
        CuAssertTrue(tc, !AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 1 == count_all_arc_obj());
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 0 == Test2_type.ao_count);
        
        arc_obj_init(&ptr2->ao_, &Test2_type);

        CuAssertTrue(tc,  AO_VALID(ptr1));
        CuAssertTrue(tc,  AO_VALID(ptr2));
        CuAssertTrue(tc, !AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 2 == count_all_arc_obj());
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        arc_obj_init(&ptr3->ao_, &Test2_type);

        CuAssertTrue(tc,  AO_VALID(ptr1));
        CuAssertTrue(tc,  AO_VALID(ptr2));
        CuAssertTrue(tc,  AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 3 == count_all_arc_obj());
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 2 == Test2_type.ao_count);

        arc_obj_deinit(&ptr2->ao_);

        CuAssertTrue(tc,  AO_VALID(ptr1));
        CuAssertTrue(tc, !AO_VALID(ptr2));
        CuAssertTrue(tc,  AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 2 == count_all_arc_obj());
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        arc_obj_deinit(&ptr1->ao_);

        CuAssertTrue(tc, !AO_VALID(ptr1));
        CuAssertTrue(tc, !AO_VALID(ptr2));
        CuAssertTrue(tc,  AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 1 == count_all_arc_obj());
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        arc_obj_init(&ptr4->ao_, &Test1_type);

        CuAssertTrue(tc, !AO_VALID(ptr1));
        CuAssertTrue(tc, !AO_VALID(ptr2));
        CuAssertTrue(tc,  AO_VALID(ptr3));
        CuAssertTrue(tc,  AO_VALID(ptr4));
        CuAssertTrue(tc, 2 == count_all_arc_obj());
        CuAssertTrue(tc, 1 == Test1_type.ao_count);
        CuAssertTrue(tc, 1 == Test2_type.ao_count);

        arc_obj_deinit(&ptr3->ao_);
        arc_obj_deinit(&ptr4->ao_);

        CuAssertTrue(tc, !AO_VALID(ptr1));
        CuAssertTrue(tc, !AO_VALID(ptr2));
        CuAssertTrue(tc, !AO_VALID(ptr3));
        CuAssertTrue(tc, !AO_VALID(ptr4));
        CuAssertTrue(tc, 0 == count_all_arc_obj());
        CuAssertTrue(tc, 0 == Test1_type.ao_count);
        CuAssertTrue(tc, 0 == Test2_type.ao_count);

        free(ptr1);
        free(ptr2);
        free(ptr3);
        free(ptr4);
    }
}