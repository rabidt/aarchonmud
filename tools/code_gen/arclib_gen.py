#!/usr/bin/env python2.7
TYPE_PAIRS = (
    ("CH", "CHAR_DATA", None, True),
    ("OBJ", "OBJ_DATA", None, True),
    ("AREA", "AREA_DATA", None, False),
    ("ROOM", "ROOM_INDEX_DATA", None, False),
    ("EXIT", "EXIT_DATA", None, False),
    ("RESET", "RESET_DATA", None, False),
    ("MOBPROTO", "MOB_INDEX_DATA", None, False),
    ("OBJPROTO", "OBJ_INDEX_DATA", None, False),
    ("PROG", "PROG_CODE", None, False),
    ("MTRIG", "PROG_LIST", "TRIG", False),
    ("OTRIG", "PROG_LIST", "TRIG", False),
    ("ATRIG", "PROG_LIST", "TRIG", False),
    ("RTRIG", "PROG_LIST", "TRIG", False),
    ("SHOP", "SHOP_DATA", None, False),
    ("AFFECT", "AFFECT_DATA", None, False),
    ("HELP", "HELP_DATA", None, False),
    ("DESCRIPTOR", "DESCRIPTOR_DATA", None, False),
    ("BOSSACHV", "BOSSACHV", None, False),
    ("BOSSREC", "BOSSREC", None, False)
)

def main():
    
    print "/* lao offset declarations */"
    ctype_seen = set()
    for ltype, ctype, tblprefix in TYPE_PAIRS:
        

        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "extern const size_t {ctype}_lao_offset;".format(ctype=ctype)

    print ""

    print "/* Type definitions */\n"
    for ltype, ctype, tblprefix, check in TYPE_PAIRS:
        if tblprefix is None:
            tblprefix = ltype

        print """static LUA_OBJ_TYPE {ltype}_type =
{{
    .type_name     = "{ltype}",
    .C_type_name   = "{ctype}",
    .p_lao_offset  = &{ctype}_lao_offset,
    .get_table     = {tblprefix}_get_table,
    .set_table     = {tblprefix}_set_table,
    .method_table  = {tblprefix}_method_table,
    .check_check   = {check_check},
    .count         = 0
}};""".format(
    ctype=ctype, ltype=ltype, tblprefix=tblprefix,
    check_check=(ltype + "_check_check") if check else "NULL")

    print "/* typesafe wrappers */\n"
    for ltype, ctype, _, _ in TYPE_PAIRS:
        print """{ctype} *check_{ltype}( lua_State *LS, int index ) {{ return arclib_check( p_{ltype}_type, LS, index ); }}
bool is_{ltype}( lua_State *LS, int index) {{ return arclib_is( p_{ltype}_type, LS, index ); }}
bool push_{ltype}( lua_State *LS, {ctype} *ud ) {{ return arclib_push( p_{ltype}_type, LS, ud ); }}
void lua_init_{ltype}( {ctype} *p ) {{ lua_arclib_obj_init( p_{ltype}_type, p ); }}
void lua_deinit_{ltype}( {ctype} *p ) {{ lua_arclib_obj_deinit( p_{ltype}_type, p ); }}
bool valid_{ltype}( {ctype} *ud ) {{ return arclib_valid( ud ); }}
int count_{ltype}( void ) {{ return arclib_count_type( p_{ltype}_type ); }}
""".format(ctype=ctype, ltype=ltype)

if __name__ == "__main__":
    main()