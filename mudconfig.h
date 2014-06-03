#define CFG_FLOAT 1
#define CFG_INT 2
#define CFG_STRING 3
#define CFG_BOOL 4
extern bool cfg_show_exp_mult;
extern bool cfg_enable_exp_mult;
extern float cfg_exp_mult;

extern bool cfg_show_qp_mult;
extern bool cfg_enable_qp_mult;
extern float cfg_qp_mult;

extern char *cfg_word_of_day;

typedef struct config_data_entry
{
    const char *name;
    int type;
    void *value;
    void *default_value;
} CFG_DATA_ENTRY;

extern CFG_DATA_ENTRY mudconfig_table[];

