#include <stddef.h>
#include <stdio.h>
#include "CuTest.h"
#include "../perfmon.h"


void Test_PERF_data_add(CuTest *tc)
{
    char buf[128];
    const int size = 100;

    PERF_data *data = PERF_data_new(size);

    int i;
    double val;
    int result;
    double total = 0;

    for (i=0; i < size; i++)
    {
        snprintf(buf, sizeof(buf), "i: %d", i);
        CuAssertIntEquals(tc, i, data->ind);

        val = 0.1 * (i + 1);
        result = PERF_data_add(data, val);

        CuAssertDblEquals(tc, val, data->vals[i], 0); 
        CuAssertIntEquals(tc, i + 1, data->count);
        if (i == (size - 1)) 
            CuAssertIntEquals(tc, 1, result);
        else
            CuAssertIntEquals(tc, 0, result);

        total += val;

        /* Why do we even need a tolerance here? They should be exactly equivalent but aren't... */
        CuAssertDblEquals_Msg(tc, buf, total, PERF_data_total(data), 0.00000001);
    }

    CuAssertIntEquals(tc, 0, data->ind);

    val = 0.1 * i;
    result = PERF_data_add(data, val);

    CuAssertDblEquals(tc, val, data->vals[0], 0); 
    CuAssertIntEquals(tc, data->size, data->count);
    CuAssertIntEquals(tc, 0, result);


    PERF_data_free(data);
}

