#!/usr/bin/env python2.7
TYPE_PAIRS = (
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

def main():
    print "/* alloc and dealloc prototypes */"
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if cname is None:
            cname = ctype

        print "{ctype} *alloc_{cname}(void);".format(
            ctype=ctype, cname=cname)
        print "void dealloc_{cname}({ctype} *ptr);".format(
            ctype=ctype, cname=cname)

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
        print "    {ctype} wrapped;".format(ctype=ctype)
        print "    struct lua_arclib_obj lao;"
        print "};"

    print ""
    print "/* lao offset definitions */"
    ctype_seen = set()
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        
        if ctype in ctype_seen:
            continue
        else:
            ctype_seen.add(ctype)

        print "const size_t {ctype}_lao_offset = offsetof(struct {ctype}_wrap, lao);".format(ctype=ctype)


    print ""
    print ""
    print "/* alloc and dealloc definitions */"
    for ltype, ctype, cname, tblprefix in TYPE_PAIRS:
        if cname is None:
            cname = ctype

        print "{ctype} *alloc_{cname}(void)".format(ctype=ctype, cname=cname)
        print "{"
        print "    struct {ctype}_wrap *wr = calloc(1, sizeof(*wr));".format(ctype=ctype)
        print "    lua_init_{ltype}(&wr->wrapped);".format(ltype=ltype)
        print "    return &wr->wrapped;"
        print "}"
        print ""
        print "void dealloc_{cname}({ctype} *p)".format(ctype=ctype, cname=cname)
        print "{"
        print "    struct {ctype}_wrap *wr = (struct {ctype}_wrap *)p;".format(ctype=ctype)
        print "    lua_deinit_{ltype}(&wr->wrapped);".format(ltype=ltype)
        print "    free(wr);"
        print "}"
        print ""


if __name__ == "__main__":
    main()