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

extern bool cfg_show_gold_mult;
extern bool cfg_enable_gold_mult;
extern float cfg_gold_mult;

extern bool cfg_refund_tattoos;
extern bool cfg_refund_qeq;

extern char *cfg_word_of_day;

extern bool cfg_show_rolls;
extern bool cfg_const_damroll;

typedef struct config_data_entry
{
    const char *name;
    int type;
    void *value;
    const void *default_value;
} CFG_DATA_ENTRY;

extern CFG_DATA_ENTRY mudconfig_table[];

