#include <sys/time.h>
#include <sys/utsname.h>
#include <gnu/libc-version.h>
#include <sqlite3.h>
#include "rapidjson/rapidjson.h"

extern "C" {
#include "merc.h"


DECLARE_DO_FUN( do_version );
} // extern "C"


DEF_DO_FUN( do_version )
{
    struct utsname buf;
    int rc = ::uname(&buf);

    if (rc < 0)
    {
        bugf("Error calling uname.");
        return;
    }
    ptc(ch, "sysname:   %s\n\r"
            "nodename:  %s\n\r"
            "release:   %s\n\r"
            "version:   %s\n\r"
            "machine:   %s\n\r"
            "\n\r",
            buf.sysname,
            buf.nodename,
            buf.release,
            buf.version,
            buf.machine);

    ptc(ch, "lua:       %s\n\r", LUA_RELEASE);

    ptc(ch, "sqlite3:   %s\n\r", sqlite3_libversion());
    ptc(ch, "rapidjson: %s\n\r", RAPIDJSON_VERSION_STRING);
#ifndef __VERSION__
    #define __VERSION__ "UNKNOWN"
#endif
    ptc(ch, "GCC:       %s\n\r", __VERSION__);
    ptc(ch, "GLIBC:     %s %s\n\r", gnu_get_libc_version(), gnu_get_libc_release());

    send_to_char(bin_info_string, ch);
}
