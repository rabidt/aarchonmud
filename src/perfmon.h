struct PERF_track;


void PERF_log_pulse(double val);
const char *PERF_repr( void );


typedef struct 
{
    int size;
    int ind;
    int count;
    double *vals;
} PERF_data;

struct PERF_meas_s;
void PERF_meas_reset( void );
void PERF_meas_start(struct PERF_meas_s **tr, const char *tag);
void PERF_meas_end(struct PERF_meas_s **tr);
const char *PERF_meas_repr( void );

#define PERF_MEASURE(name, section) \
struct PERF_meas_s *_ms_ ## name;\
PERF_meas_start(& _ms_ ## name, #name);\
section \
PERF_meas_end(& _ms_ ## name);


#ifdef UNITTEST
PERF_data *PERF_data_new(int size);
int PERF_data_add(PERF_data *data, double val);
void PERF_data_free(PERF_data *data);
double PERF_data_total(PERF_data *data);
#endif
