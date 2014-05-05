#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "mudconfig.h"

bool cfg_enable_exp_mult=FALSE;

float cfg_exp_mult;
const float cfg_exp_mult_default=1;

char *cfg_word_of_day;
const char *cfg_word_of_day_default="bananahammock";

CFG_DATA_ENTRY mudconfig_table[] =
{
    { "enable_exp_mult", CFG_BOOL, &cfg_enable_exp_mult, NULL }, 
    { "exp_mult", CFG_FLOAT, &cfg_exp_mult, &cfg_exp_mult_default },
    { "word_of_day", CFG_STRING, &cfg_word_of_day, &cfg_word_of_day_default},
    { NULL, NULL, NULL, NULL }
};
/*
void do_mudconfig( CHAR_DATA *ch, char *argument)
{
    struct config_data_entry *en;
    int i;
    for ( i=0 ; mudconfig_table[i].name ; i++ )
    {
        en=&mudconfig_table[i];

        char fmt;
        switch(en->type)
        {
            case CFG_FLOAT:
            {
                ptc( ch, "%-50s %f\n\r", en->name,  *((float *)(en->value)));
                break;
            }
            case CFG_INT:
            {
                ptc( ch, "%-50s %d\n\r", en->name, *((int *)(en->value)));
                break;
            }
            case CFG_STRING:
            {
                ptc( ch, "%-50s %s\n\r", en->name, *((char **)(en->value)));
                break;
            }
            case CFG_BOOL:
            {
                ptc( ch, "%-50s %s\n\r", en->name, *((bool *)(en->value)) ? "TRUE":"FALSE");
                break;
            }
            default:
            {
                bugf("Bad type in do_mudconfig.");
                return;
            }
        }

    }
}
*/
void mudconfig_init()
{
    /* set defaults, especially important for strings, others can
       really just be set in declaration, but can also be set with
       default_value fields */
    int i;
    CFG_DATA_ENTRY *en;

    for ( i=0; mudconfig_table[i].name ; i++ )
    {
        en=&mudconfig_table[i];

        if ( en->default_value )
        {
            switch(en->type)
            {
                case CFG_INT:
                {
                    *((int *)(en->value))=*((int *)(en->default_value));
                    break;
                }
                case CFG_FLOAT:
                {
                    *((float *)(en->value))=*((float *)(en->default_value));
                    break;
                }
                case CFG_BOOL:
                {
                    *((bool *)(en->value))=*((bool *)(en->default_value));
                    break;
                }
                case CFG_STRING:
                {
                    *((char **)(en->value))=str_dup( *((char **)(en->default_value)) );
                    break;
                }
            }
        }
    }

    /* double check that strings have defaults, otherwise give it one */
    for ( i=0; mudconfig_table[i].name ; i++ )
    {
        en=&mudconfig_table[i];

        if (en->type==CFG_STRING)
        {
            if ( *((char **)(en->value)) == NULL )
            {
                bugf("mudconfig_init: no default value for %s", en->name );
                *((char **)(en->value))=str_dup("");
            }
        }
    }

    return;
}    
