#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include "merc.h"
#include "lua_help.h"
//typedef struct luahelper LUAHELPER;
typedef struct lh_section LH_SECTION;
typedef struct topic_node TOPIC_NODE;


struct lh_section
{
    char *name;
    LH_SECTION *next;
    TOPIC_NODE *topics;
};

struct topic_node
{
    TOPIC_NODE *next;
    LUAHELP_TOPIC *topic;
};

LH_SECTION *s_sections=NULL;

static LH_SECTION *new_section( const char *name )
{
    LH_SECTION *new=alloc_mem( sizeof(LH_SECTION ) );

    new->topics=NULL;
    new->name=str_dup(name);

    new->next=s_sections;
    s_sections=new;

    return new;
}

static TOPIC_NODE *new_topic_node( LUAHELP_TOPIC *topic )
{
    TOPIC_NODE *new=alloc_mem( sizeof(TOPIC_NODE) );

    new->topic=topic;

    return new;
}

static void addtosec( LUAHELP_TOPIC *topic, LH_SECTION *sec )
{
    TOPIC_NODE *tn=new_topic_node(topic);

    tn->next=sec->topics;
    sec->topics=tn->next;

    return;
}

void add_topic( const char *section, LUAHELP_TOPIC *topic )
{
    LH_SECTION *sec=NULL;

    for ( sec=s_sections; sec ; sec=sec->next )
    {
        if (!strcmp( section, sec->name) )
        {
            addtosec( topic, sec );
            return;
        }
    }

    /* section doesn't exist yet */
    sec=new_section( section );
    addtosec( topic, sec );
    return;

}

void do_luahelp( CHAR_DATA *ch, const char *argument )
{
    /*TBC*/
}


