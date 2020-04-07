#!/usr/bin/env python2.7
TYPE_PAIRS_ARC = (
    # ltype, ctype, cname, tblprefix
    ("CH", "CHAR_DATA", None, None),
    ("OBJ", "OBJ_DATA", None, None),
    ("AREA", "AREA_DATA", None, None),
    ("ROOM", "ROOM_INDEX_DATA", None, None),
    ("EXIT", "EXIT_DATA", None, None),
    ("RESET", "RESET_DATA", None, None),
    ("MOBPROTO", "MOB_INDEX_DATA", None, None),
    ("OBJPROTO", "OBJ_INDEX_DATA", None, None),
    ("PROG", "PROG_CODE", None, None),
    ("MTRIG", "PROG_LIST", "MPROG_LIST", "TRIG"),
    ("OTRIG", "PROG_LIST", "OPROG_LIST", "TRIG"),
    ("ATRIG", "PROG_LIST", "APROG_LIST", "TRIG"),
    ("RTRIG", "PROG_LIST", "RPROG_LIST", "TRIG"),
    ("SHOP", "SHOP_DATA", None, None),
    ("AFFECT", "AFFECT_DATA", None, None),
    ("HELP", "HELP_DATA", None, None),
    ("DESCRIPTOR", "DESCRIPTOR_DATA", None, None),
    ("BOSSACHV", "BOSSACHV", None, None),
    ("BOSSREC", "BOSSREC", None, None)
)


TYPE_PAIRS_TEST = (
    (None, "Test1", None, None),
    (None, "Test2", None, None)
)

def main():
    import sys
    is_test = len(sys.argv) > 1 and sys.argv[1] == "test"
    if is_test:
        TYPE_PAIRS = TYPE_PAIRS_TEST
    else:
        TYPE_PAIRS = TYPE_PAIRS_ARC

    print "/* alloc and dealloc prototypes */"
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if cname is None:
            cname = ctype

        print "{ctype} *alloc_{cname}(void);".format(
            ctype=ctype, cname=cname)
        print "void dealloc_{cname}({ctype} *ptr);".format(
            ctype=ctype, cname=cname)

    print ""
    print "/* valid struct prototypes */"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "bool is_valid_{ctype}({ctype} *p);".format(ctype=ctype)


    print ""
    print "/* wrap structs */"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "struct {ctype}_wrap".format(ctype=ctype)
        print "{"
        print "    struct arc_obj aoh;"
        print "    {ctype} wrapped;".format(ctype=ctype)
        print "    struct arc_obj aot;"
        if ltype:
            print "    struct lua_arclib_obj lao;"
        print "};"

    print ""
    print "/* arc_obj_type definitions */"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "static struct arc_obj_type {ctype}_type = ".format(ctype=ctype)
        print "{"
        print "    .ao_count = 0,"
        print "    .name = \"{ctype}\",".format(ctype=ctype)
        print "    .aoh_offset = offsetof(struct {ctype}_wrap, aoh),".format(ctype=ctype)
        print "    .wrapped_offset = offsetof(struct {ctype}_wrap, wrapped),".format(ctype=ctype)
        print "    .aot_offset = offsetof(struct {ctype}_wrap, aot),".format(ctype=ctype)
        print "};"
    
    print ""
    print "static struct arc_obj_type *all_arc_obj_types[] ="
    print "{"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "    &{ctype}_type,".format(ctype=ctype)
    print "    NULL"
    print "};"


    print ""
    print "/* lao offset definitions */"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if not ltype:
            continue
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "const size_t {ctype}_lao_offset = offsetof(struct {ctype}_wrap, lao) - offsetof(struct {ctype}_wrap, wrapped);".format(ctype=ctype)


    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print """
static struct {ctype}_wrap *{ctype}_get_wrap({ctype} *p)
{{
    struct {ctype}_wrap *wr = (struct {ctype}_wrap *)((uintptr_t)p - offsetof(struct {ctype}_wrap, wrapped));
    return wr;
}}""".format(ctype=ctype)

    print ""
    print ""
    print "/* alloc and dealloc definitions */"
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if cname is None:
            cname = ctype

        print "{ctype} *alloc_{cname}(void)".format(ctype=ctype, cname=cname)
        print "{"
        print "    struct {ctype}_wrap *wr = calloc(1, sizeof(*wr));".format(ctype=ctype)
        print "    arc_obj_init(&{ctype}_type, &wr->aoh, &wr->aot);".format(ctype=ctype)
        if ltype:
            print "    lua_init_{ltype}(&wr->wrapped);".format(ltype=ltype)
        print "    return &wr->wrapped;"
        print "}"
        print ""
        print "void dealloc_{cname}({ctype} *p)".format(ctype=ctype, cname=cname)
        print "{"
        print "    struct {ctype}_wrap *wr = {ctype}_get_wrap(p);".format(ctype=ctype)
        if ltype:
            print "    lua_deinit_{ltype}(&wr->wrapped);".format(ltype=ltype)
        print "    arc_obj_deinit(&{ctype}_type, &wr->aoh, &wr->aot);".format(ctype=ctype)
        print "    free(wr);"
        print "}"
        print ""

    print ""
    print "/* is_valid struct definitions */"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "bool is_valid_{ctype}({ctype} *p) {{ return is_valid_arc_obj(&{ctype}_type, p); }}".format(ctype=ctype)



if __name__ == "__main__":
    main()