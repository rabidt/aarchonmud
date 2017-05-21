#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
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


static struct 
{
    int const threshold;
    unsigned long count;
} threshold_info [] =
{
    /* Must be in ascending order */
    {10,    0},
    {30,    0},
    {50,    0},
    {70,    0},
    {90,    0},
    {100,   0},
    {250,   0},
    {500,   0},
    {1000,  0},
    {2500,  0}
};

void PERF_meas_reset( void )
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

const char *PERF_meas_repr( void )
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
    d->avgs = malloc(sizeof(double) * size);
    d->mins = malloc(sizeof(double) * size);
    d->maxes = malloc(sizeof(double) * size);

    return d;
}

static void init( void )
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

static void check_init( void )
{
    if (init_done)
        return;
    
    init();

    init_done = 1;
}

#ifndef UNITTEST
static
#endif
int PERF_data_add(PERF_data *data, double avg, double min, double max)
{
    data->avgs[data->ind] = avg;
    data->mins[data->ind] = min;
    data->maxes[data->ind] = max;

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
double PERF_data_avg_avg(PERF_data *data)
{
    double sum = 0;
    int i;

    for (i=0; i < data->count; ++i)
    {
        sum += data->avgs[i];
    }

    return sum / data->count;
}

#ifndef UNITTEST
static
#endif
double PERF_data_min_min(PERF_data *data)
{
    double min = INFINITY;
    int i;

    for (i=0; i < data->count; ++i)
    {
        if (data->mins[i] < min)
        {
            min = data->mins[i];
        }
    }

    return min;
}

#ifndef UNITTEST
static
#endif
double PERF_data_max_max(PERF_data *data)
{
    double max = 0;
    int i;

    for (i=0; i < data->count; ++i)
    {
        if (data->maxes[i] > max)
        {
            max = data->maxes[i];
        }
    }

    return max;
}

static void check_thresholds(double val)
{
    const unsigned int thresh_count = sizeof(threshold_info) / sizeof(threshold_info[0]);

    unsigned int i;

    for (i=0; i < thresh_count; ++i)
    {
        if (val > threshold_info[i].threshold)
        {
            ++(threshold_info[i].count);
        }
        else
        {
            break; //  Array should be in ascending order
        }
    }
}

void PERF_log_pulse(double val)
{
    check_init();

    last_pulse = val;
    if (val > max_pulse)
        max_pulse = val;
    
    check_thresholds(val);
    
    if (!PERF_data_add(pulse_data, val, val, val))
        return;

    if (!PERF_data_add(
            sec_data, 
            PERF_data_avg_avg(pulse_data),
            PERF_data_min_min(pulse_data),
            PERF_data_max_max(pulse_data)))
        return;

    if (!PERF_data_add(
            min_data, 
            PERF_data_avg_avg(sec_data),
            PERF_data_min_min(sec_data),
            PERF_data_max_max(sec_data)))
        return;

   if (!PERF_data_add(
            hour_data, 
            PERF_data_avg_avg(min_data),
            PERF_data_min_min(min_data),
            PERF_data_max_max(min_data)))
        return;

    day_count += 1;

}

const char *PERF_repr( void )
{
    const unsigned int thresh_count = sizeof(threshold_info) / sizeof(threshold_info[0]);
    unsigned int i;
    static char buf[1024];
    unsigned int offset = 0;
    time_t total_secs = time(NULL) - init_time;
    double total_pulses = total_secs * PULSE_PER_SECOND;

    offset = snprintf(buf, sizeof(buf),
        "                       Avg         Min         Max\n\r"
        "  %3d Pulse:   %10.2f%% %10.2f%% %10.2f%%\n\r"
        "  %3d Pulses:  %10.2f%% %10.2f%% %10.2f%%\n\r"
        "  %3d Seconds: %10.2f%% %10.2f%% %10.2f%%\n\r"
        "  %3d Minutes: %10.2f%% %10.2f%% %10.2f%%\n\r"
        "  %3d Hours:   %10.2f%% %10.2f%% %10.2f%%\n\r"
        "\n\r"
        "Max pulse:     %.2f%%\n\r"
        "\n\r",
        1, last_pulse, last_pulse, last_pulse,
        
        pulse_data->count, 
        PERF_data_avg_avg(pulse_data), 
        PERF_data_min_min(pulse_data), 
        PERF_data_max_max(pulse_data),

        sec_data->count, 
        PERF_data_avg_avg(sec_data), 
        PERF_data_min_min(sec_data), 
        PERF_data_max_max(sec_data),

        min_data->count, 
        PERF_data_avg_avg(min_data), 
        PERF_data_min_min(min_data), 
        PERF_data_max_max(min_data),

        hour_data->count, 
        PERF_data_avg_avg(hour_data), 
        PERF_data_min_min(hour_data), 
        PERF_data_max_max(hour_data),

        max_pulse    
    );

    for (i=0; i < thresh_count; ++i)
    {
        if (offset >= (sizeof(buf) - 1) )
        {
            break;
        }
        offset += snprintf(buf + offset, (sizeof(buf) - offset), 
            "Over %5d%%:    %.2f%% (%ld)\n\r",
            threshold_info[i].threshold,
            threshold_info[i].count / total_pulses,
            threshold_info[i].count);
    }


    return buf;
}
