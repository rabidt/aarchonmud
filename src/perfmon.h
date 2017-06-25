#ifndef PERFMON_H


void PERF_log_pulse(double val);
void PERF_repr( char *out_buf, size_t n );

struct PERF_meas_s;
void PERF_meas_reset( void );
void PERF_meas_start(struct PERF_meas_s **tr, const char *tag);
void PERF_meas_end(struct PERF_meas_s **tr);
void PERF_meas_repr( char *out_buf, size_t n );

#define PERF_MEASURE(name, section) \
struct PERF_meas_s *_ms_ ## name;\
PERF_meas_start(& _ms_ ## name, #name);\
section \
PERF_meas_end(& _ms_ ## name);


struct PERF_prof_sect;

void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id);
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr);
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr);
void PERF_prof_reset( void );
void PERF_prof_repr( char *out_buf, size_t n );


#endif // PERFMON_H
