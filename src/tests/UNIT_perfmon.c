#include <stddef.h>
#include <stdio.h>
#include "CuTest.h"
#include "../perfmon.h"


static void check_total(CuTest *tc, PERF_data *data)
{
    int i;
    double total = 0;

    for (i=0; i < data->count; i++)
    {
        total += data->vals[i];
    } 

    CuAssertDblEquals(tc, total, data->total, 0.00000001);
}

void Test_PERF_data_add(CuTest *tc)
{
    const int size = 100;

    PERF_data *data = PERF_data_new(size);

    int i;
    double val;
    int result;

    for (i=0; i < size; i++)
    {
        CuAssertIntEquals(tc, i, data->ind);

        val = 0.1 * (i + 1);
        result = PERF_data_add(data, val);

        CuAssertDblEquals(tc, val, data->vals[i], 0); 
        CuAssertIntEquals(tc, i + 1, data->count);
        if (i == (size - 1)) 
            CuAssertIntEquals(tc, 1, result);
        else
            CuAssertIntEquals(tc, 0, result);

        check_total(tc, data);
    }

    CuAssertIntEquals(tc, 0, data->ind);

    val = 0.1 * i;
    result = PERF_data_add(data, val);

    CuAssertDblEquals(tc, val, data->vals[0], 0); 
    CuAssertIntEquals(tc, data->size, data->count);
    CuAssertIntEquals(tc, 0, result);


    PERF_data_free(data);
}

