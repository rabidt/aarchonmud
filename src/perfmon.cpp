#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <cfloat>
#include <cstring>

extern "C" {
#include <sys/time.h>
#include "perfmon.h"
} // extern "C"

//static unsigned int const INFINITY = (unsigned int)(-1);
static double const INFINITY = DBL_MAX;

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
    char *tag;
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
        free(static_cast<void *>(ptr->tag));
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

void PERF_meas_repr( char *out_buf, size_t n )
{
    std::ostringstream os;
    os << std::setprecision(2) << std::fixed;

    for ( int i=0; i < meas_ind; i++ )
    {
        int lvl = measurements[i].level;
        for ( int j = 0 ; j < lvl ; ++j )
        {
            os << "  ";
        }
        if ( lvl > 0 )
        {
            os << "|-";
        }

        long elapsed = (measurements[i].end.tv_sec  - measurements[i].start.tv_sec )*1000000 + 
                       (measurements[i].end.tv_usec - measurements[i].start.tv_usec);

        if ( measurements[i].tag )
        {
            os << std::left << std::setw(20) << measurements[i].tag;    
        }
        os << " - " 
           << std::setw(20) << ( 100 * (static_cast<double>(elapsed) / USEC_PER_PULSE))
           << "\n\r";

        
    }

    std::string str = os.str();
    size_t copied = str.copy( out_buf, n - 1 );
    out_buf[copied] = '\0';
}

static int init_done = 0;

static time_t init_time;

static double last_pulse;
static double max_pulse = 0;


class PerfIntvlData
{
public:
    explicit PerfIntvlData( size_t size, PerfIntvlData *next )
        : mSize( size )
        , mInd( 0 )
        , mCount( 0 )
        , mAvgs( size )
        , mMins( size )
        , mMaxes( size )
        , mpNextIntvl( next )
    {

    }

    void AddData(double avg, double min, double max);
    double GetAvgAvg() const;
    double GetMinMin() const;
    double GetMaxMax() const;

    size_t GetCount() const { return mCount; }

private:
    PerfIntvlData( const PerfIntvlData & );
    PerfIntvlData & operator=( const PerfIntvlData & );

    size_t mSize;
    size_t mInd;
    size_t mCount;

    std::vector<double> mAvgs;
    std::vector<double> mMins;
    std::vector<double> mMaxes;

    PerfIntvlData *mpNextIntvl;
};

static PerfIntvlData sHourData( HOUR_PER_DAY, NULL );
static PerfIntvlData sMinuteData( MIN_PER_HOUR, &sHourData );
static PerfIntvlData sSecData( SEC_PER_MIN, &sMinuteData );
static PerfIntvlData sPulseData( PULSE_PER_SECOND, &sPulseData );


void 
PerfIntvlData::AddData(double avg, double min, double max)
{
    mAvgs[mInd] = avg;
    mMins[mInd] = min;
    mMaxes[mInd] = max;

    if ( mCount <= mInd )
    {
        mCount = mInd + 1;
    }

    if ( mInd < (mSize - 1) )
    {
        mInd += 1;
    }
    else
    {
        mInd = 0;

        if ( mpNextIntvl )
        {
            mpNextIntvl->AddData(
                this->GetAvgAvg(),
                this->GetMinMin(),
                this->GetMaxMax());
        }
    }
}

double
PerfIntvlData::GetAvgAvg() const
{
    double sum = 0;

    for ( size_t i = 0 ; i < mCount ; ++i)
    {
        sum += mAvgs[i];
    }

    return sum / mCount;
}

double
PerfIntvlData::GetMinMin() const
{
    double min = INFINITY;

    for ( size_t i = 0 ; i < mCount ; ++i )
    {
        if ( mMins[i] < min )
        {
            min = mMins[i];
        }
    }

    return min;
}

double
PerfIntvlData::GetMaxMax() const
{
    double max = 0;

    for ( size_t i = 0 ; i < mCount ; ++i )
    {
        if ( mMaxes[i] > max )
        {
            max = mMaxes[i];
        }
    }

    return max;   
}

static void init( void )
{
    init_time = time(NULL);
}

static void check_init( void )
{
    if (init_done)
        return;
    
    init();

    init_done = 1;
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
    
    sPulseData.AddData( val, val, val );
}

void PERF_repr( char *out_buf, size_t n )
{
    if ( !out_buf )
        return;
    if ( n < 1 )
        return;

    std::ostringstream os;

    const unsigned int thresh_count = sizeof(threshold_info) / sizeof(threshold_info[0]);
    unsigned int i;
    time_t total_secs = time(NULL) - init_time;
    double total_pulses = total_secs * PULSE_PER_SECOND;

    double pulse_min = sPulseData.GetMinMin();
    pulse_min = (pulse_min == INFINITY) ? 0 : pulse_min;

    double sec_min = sSecData.GetMinMin();
    sec_min = (sec_min == INFINITY) ? 0 : sec_min;

    double min_min = sMinuteData.GetMinMin();
    min_min = (min_min == INFINITY) ? 0 : min_min;

    double hour_min = sHourData.GetMinMin();
    hour_min = (hour_min == INFINITY) ? 0 : hour_min;

    os << std::fixed << std::setprecision(2)
       << "                     Avg         Min         Max\n\r"
       << std::setw(3) << 1 << " Pulse:   " 
       << std::setw(10) << last_pulse << "% " 
       << std::setw(10) << last_pulse << "% " 
       << std::setw(10) << last_pulse << "%\n\r"

       << std::setw(3) << sPulseData.GetCount() << " Pulses:  " 
       << std::setw(10) << sPulseData.GetAvgAvg() << "% "
       << std::setw(10) << pulse_min << "% "
       << std::setw(10) << sPulseData.GetMaxMax() << "%\n\r"

       << std::setw(3) << sSecData.GetCount() << " Seconds: " << std::setw(10)
       << std::setw(10) << sSecData.GetAvgAvg() << "% "
       << std::setw(10) << sec_min << "% "
       << std::setw(10) << sSecData.GetMaxMax() << "%\n\r"

       << std::setw(3) << sMinuteData.GetCount() << " Minutes: " << std::setw(10)
       << std::setw(10) << sMinuteData.GetAvgAvg() << "% "
       << std::setw(10) << min_min << "% "
       << std::setw(10) << sMinuteData.GetMaxMax() << "%\n\r"

       << std::setw(3) << sHourData.GetCount() << " Hours:   " << std::setw(10)
       << std::setw(10) << sHourData.GetAvgAvg() << "% "
       << std::setw(10) << hour_min << "% "
       << std::setw(10) << sHourData.GetMaxMax() << "%\n\r"

       << "\n\rMax pulse:      " << max_pulse << "\n\r\n\r";
  

    for (i=0; i < thresh_count; ++i)
    {
        os << "Over " << std::setw(5)  
           << threshold_info[i].threshold << "%:      "
           << (threshold_info[i].count / total_pulses) << "% "
           << "(" << threshold_info[i].count << ")\n\r";
    }

    

    std::string str = os.str();
    size_t copied = str.copy( out_buf, n - 1 );
    out_buf[copied] = '\0';
}
