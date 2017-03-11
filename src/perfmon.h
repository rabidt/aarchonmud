void PERF_log_pulse(double val);
const char *PERF_repr();


typedef struct 
{
    int size;
    int ind;
    int count;
    double total;
    double *vals;
} PERF_data;


#ifdef UNITTEST
PERF_data *PERF_data_new(int size);
int PERF_data_add(PERF_data *data, double val);
void PERF_data_free(PERF_data *data);
#endif
