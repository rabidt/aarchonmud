#include <stdlib.h>
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
        PERF_PROF_ENTER( _ms_main, "main");
        
        gettimeofday(&start_time, NULL);
        
        PERF_prof_reset();

        PERF_PROF_ENTER( _ms_fopen, "fopen" );
        fp = fopen("dummy.txt", "w");
        PERF_PROF_EXIT( _ms_fopen );

        PERF_PROF_ENTER( _ms_writes, "writes");
        int i;
        for (i=0; i < 500; i++)
        {
            fprintf(fp, line);
        }
        PERF_PROF_EXIT( _ms_writes );

        PERF_PROF_ENTER( _ms_fclose, "fclose" );
        fclose(fp);
        PERF_PROF_EXIT( _ms_fclose );

        PERF_PROF_ENTER( _ms_echo1, "echo1");
        system("echo HELLO > /dev/null");
        PERF_PROF_EXIT( _ms_echo1 );
        
        PERF_PROF_EXIT( _ms_main );
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

            char buf[2048];
            PERF_repr( buf, sizeof(buf) );
            printf(buf);
            fflush(stdout);
        }
        struct timeval stall_time;
        stall_time.tv_usec = 250000;
        stall_time.tv_sec = 0;

        select(0, NULL, NULL, NULL, &stall_time);
    }

    return 0;
}
