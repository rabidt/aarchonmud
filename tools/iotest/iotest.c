#include <stdio.h>
#include <sys/time.h>
#include "../../src/perfmon.h"

const char *line = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n";

int main()
{
    FILE *fp;
    
    long iter = 0;

    struct timeval start_time;
    struct timeval end_time;

    while (1)
    {
        iter++;
        struct PERF_meas_s *ms_main;
        struct PERF_meas_s *ms_writes;
        
        gettimeofday(&start_time, NULL);
        
        PERF_meas_reset();

        PERF_meas_start(&ms_main, "main");
        PERF_MEASURE(fopen,
                fp = fopen("dummy.txt", "w"););

        PERF_meas_start(&ms_writes, "writes");
        int i;
        for (i=0; i < 500; i++)
        {
            fprintf(fp, line);
        }
        PERF_meas_end(&ms_writes);
        PERF_MEASURE(fclose,
                fclose(fp););

        PERF_MEASURE(echo1,
                system("echo HELLO > /dev/null"););
        
        PERF_meas_end(&ms_main); 
        gettimeofday(&end_time, NULL);

        long elapsed = (end_time.tv_sec  - start_time.tv_sec)*1000000 +
                       (end_time.tv_usec - start_time.tv_usec);

        double usage = 100 * (double)elapsed / 250000;
        PERF_log_pulse(usage);
        if (elapsed >= 250000)
        {
            printf("--------------------------------------------------\n");
            printf("Iteration %ld\n", iter);
            printf("Elapsed: %ld\n", elapsed);
            printf(PERF_meas_repr());
            printf(PERF_repr());
            fflush(stdout);
        }
        struct timeval stall_time;
        stall_time.tv_usec = 250000;
        stall_time.tv_sec = 0;

        select(0, NULL, NULL, NULL, &stall_time);
    }

    return 0;
}
