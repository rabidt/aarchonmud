#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "perfmon.h"

#define PULSE_PER_SECOND 4
#define SEC_PER_MIN 60
#define PULSE_PER_MIN (PULSE_PER_SECOND * SEC_PER_MIN)
#define MIN_PER_HOUR 60
#define PULSE_PER_HOUR (PULSE_PER_MIN * MIN_PER_HOUR)
#define HOUR_PER_DAY 24
#define PULSE_PER_DAY (PULSE_PER_HOUR * HOUR_PER_DAY)

#define USEC_PER_PULSE (1000000 / PULSE_PER_SECOND)

#define MAX_MEAS 128 

struct PERF_meas_s 
{
    int index;
    int level;
    const char *tag;
    struct timeval start;
    struct timeval end;
};


static struct PERF_meas_s measurements[MAX_MEAS];
static int meas_ind = 0;
static int curr_level = 0;

void PERF_meas_reset()
{
    meas_ind = 0;
}

void PERF_meas_start(struct PERF_meas_s **m, const char *tag)
{
    if (meas_ind >= MAX_MEAS)
        return; // TODO: return an error

    struct PERF_meas_s *ptr = &(measurements[meas_ind]);

    ptr->index = meas_ind;
    ptr->level = curr_level++;
    if (ptr->tag)
    {
        free((void *)ptr->tag);
    }
    if (tag)
    {
        ptr->tag = strdup(tag);
    }
    else
    {
        ptr->tag = NULL;
    }

    gettimeofday(&(ptr->start), NULL);

    meas_ind += 1;

    *m = ptr; 
}

void PERF_meas_end(struct PERF_meas_s **m)
{
    gettimeofday(&((*m)->end), NULL);
    curr_level--;
    *m = NULL;
}

const char *PERF_meas_repr()
{
    static char buf[MAX_MEAS * 80];
    // assume max depth 10
    
    int offset = 0;

    int i;
    for (i=0; i < meas_ind; i++)
    {
        char indent[] = "                    ";
        int lvl = measurements[i].level;

        if ((lvl*2) < sizeof(indent))
        {
            indent[lvl*2] = '\0';
            if (lvl > 0)
            {
               indent[lvl*2-1] = '-';
               indent[lvl*2-2] = '|';
            } 
        } 

        long elapsed = (measurements[i].end.tv_sec  - measurements[i].start.tv_sec )*1000000 + 
                       (measurements[i].end.tv_usec - measurements[i].start.tv_usec);
        offset += snprintf(
                buf + offset, 
                80, "%s%-20s - %.2f%%\n\r", 
                indent,
                measurements[i].tag ? measurements[i].tag : "", 
                100 * ((double)elapsed / USEC_PER_PULSE));
    }

    return buf;
}

static int init_done = 0;

static time_t init_time;

static double last_pulse;
static double max_pulse = 0;
static unsigned long over100_count = 0;
static unsigned long over90_count = 0;
static unsigned long over70_count = 0;
static unsigned long over50_count = 0;
static unsigned long over30_count = 0;
static unsigned long over10_count = 0;
static unsigned long day_count = 0;

static PERF_data *pulse_data;
static PERF_data *sec_data;
static PERF_data *min_data;
static PERF_data *hour_data;

#ifndef UNITTEST
static 
#endif
PERF_data *PERF_data_new(int size)
{
    PERF_data *d = malloc(sizeof(PERF_data));
    d->size = size;
    d->ind = 0;
    d->count = 0;
    d->vals = malloc(sizeof(double) * size);

    return d;
}

static void init()
{
    init_time = time(NULL);

    pulse_data = PERF_data_new(PULSE_PER_SECOND);
    sec_data = PERF_data_new(SEC_PER_MIN);
    min_data = PERF_data_new(MIN_PER_HOUR);
    hour_data = PERF_data_new(HOUR_PER_DAY);
}

#ifdef UNITTEST
void PERF_data_free(PERF_data *data)
{
    free(data->vals);
    free(data);
}
#endif

static void check_init()
{
    if (init_done)
        return;
    
    init();

    init_done = 1;
}

#ifndef UNITTEST
static
#endif
int PERF_data_add(PERF_data *data, double val)
{
    data->vals[data->ind] = val;

    if (data->count <= data->ind)
        data->count = data->ind + 1;

    if (data->ind < (data->size - 1))
    {
        data->ind += 1;
        return 0;
    }
    else
    {
        data->ind = 0;
        return 1;
    }
}

#ifndef UNITTEST
static
#endif
double PERF_data_total(PERF_data *data)
{
    double rtn = 0;
    int i;

    for (i=0; i < data->count; i++)
    {
        rtn += data->vals[i];
    }

    return rtn;
}

#define AVG(data) ( PERF_data_total(data) / data->count )
void PERF_log_pulse(double val)
{
    check_init();

    last_pulse = val;
    if (val > max_pulse)
        max_pulse = val;
    
    if (val > 100)
        over100_count += 1;
    if (val > 90)
        over90_count += 1;
    if (val > 70)
        over70_count += 1;
    if (val > 50)
        over50_count += 1; 
    if (val > 30)
        over30_count += 1;
    if (val > 10)
        over10_count += 1;


    if (!PERF_data_add(pulse_data, val))
        return;

    double sec_val = AVG(pulse_data);

    if (!PERF_data_add(sec_data, sec_val))
        return;

    double min_val = AVG(sec_data);

    if (!PERF_data_add(min_data, min_val))
        return;

    double hour_val = AVG(min_data);

    if (!PERF_data_add(hour_data, hour_val))
        return;

    day_count += 1;

}

const char *PERF_repr()
{
    static char buf[512];

    time_t total_secs = time(NULL) - init_time;
    double total_pulses = total_secs * PULSE_PER_SECOND;

    snprintf(buf, sizeof(buf),
        "Averages\n\r"
        "  %3d Pulse:   %.2f%%\n\r"
        "  %3d Pulses:  %.2f%%\n\r"
        "  %3d Seconds: %.2f%%\n\r"
        "  %3d Minutes: %.2f%%\n\r"
        "  %3d Hours:   %.2f%%\n\r"
        "\n\r"
        "Max pulse:     %.2f%%\n\r"
        "\n\r"
        "Over  10%%:    %.2f%% (%ld)\n\r"
        "Over  30%%:    %.2f%% (%ld)\n\r"
        "Over  50%%:    %.2f%% (%ld)\n\r"
        "Over  70%%:    %.2f%% (%ld)\n\r"
        "Over  90%%:    %.2f%% (%ld)\n\r"
        "Over 100%%:    %.2f%% (%ld)\n\r",
        1, last_pulse,
        pulse_data->count, AVG(pulse_data),
        sec_data->count, AVG(sec_data),
        min_data->count, AVG(min_data),
        hour_data->count, AVG(hour_data),
        max_pulse,
        over10_count * 100 / total_pulses, over10_count,
        over30_count * 100 / total_pulses, over30_count,
        over50_count * 100 / total_pulses, over50_count,
        over70_count * 100 / total_pulses, over70_count,
        over90_count * 100 / total_pulses, over90_count,
        over100_count * 100 / total_pulses, over100_count
    );

    return buf;
}
#undef AVG
