#!/usr/bin/env python2.7
TYPE_PAIRS = (
    ("CH", "CHAR_DATA", None),
    ("OBJ", "OBJ_DATA", None),
    ("AREA", "AREA_DATA", None),
    ("ROOM", "ROOM_INDEX_DATA", None),
    ("EXIT", "EXIT_DATA", None),
    ("RESET", "RESET_DATA", None),
    ("MOBPROTO", "MOB_INDEX_DATA", None),
    ("OBJPROTO", "OBJ_INDEX_DATA", None),
    ("PROG", "PROG_CODE", None),
    ("MTRIG", "PROG_LIST", "TRIG"),
    ("OTRIG", "PROG_LIST", "TRIG"),
    ("ATRIG", "PROG_LIST", "TRIG"),
    ("RTRIG", "PROG_LIST", "TRIG"),
    ("SHOP", "SHOP_DATA", None),
    ("AFFECT", "AFFECT_DATA", None),
    ("HELP", "HELP_DATA", None),
    ("DESCRIPTOR", "DESCRIPTOR_DATA", None),
    ("BOSSACHV", "BOSSACHV", None),
    ("BOSSREC", "BOSSREC", None)
)

def main():
    print "/* Type definitions */\n"
    for ltype, ctype, tblprefix in TYPE_PAIRS:
        if tblprefix is None:
            tblprefix = ltype

        print """static LUA_OBJ_TYPE {ltype}_type =
{{
    .type_name     = "{ltype}",
    .C_type_name   = "{ctype}",
    .C_struct_size = sizeof({ctype}),
    .get_table     = {tblprefix}_get_table,
    .set_table     = {tblprefix}_set_table,
    .method_table  = {tblprefix}_method_table,
    .count         = 0
}};
""".format(ctype=ctype, ltype=ltype, tblprefix=tblprefix)

    print "/* typesafe wrappers */\n"
    for ltype, ctype, _ in TYPE_PAIRS:
        print """{ctype} *check_{ltype}( lua_State *LS, int index ) {{ return arclib_check( p_{ltype}_type, LS, index ); }}
bool is_{ltype}( lua_State *LS, int index) {{ return arclib_is( p_{ltype}_type, LS, index ); }}
bool push_{ltype}( lua_State *LS, {ctype} *ud ) {{ return arclib_push( p_{ltype}_type, LS, ud ); }}
{ctype} *alloc_{ltype}( void ) {{ return arclib_alloc( p_{ltype}_type ); }}
void free_{ltype}( {ctype} *ud ) {{ arclib_free( p_{ltype}_type, ud ); }}
bool valid_{ltype}( {ctype} *ud ) {{ return arclib_valid( ud ); }}
int count_{ltype}( void ) {{ return arclib_count_type( p_{ltype}_type); }}
""".format(ctype=ctype, ltype=ltype)

if __name__ == "__main__":
    main()