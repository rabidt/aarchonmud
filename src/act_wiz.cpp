#include <string>
#include <sstream>
extern "C" {
#include <sys/time.h>
#include "merc.h"
} // extern "C"





extern "C" {
DEF_DO_FUN(do_cpptest)
{
    std::stringstream strm;





    for (int i=0; i < 100;  ++i)
    {
        strm << "Ribbit asdfffffffffffffffffffffffffffffffffdddddddddddddddddddddddddddddddddddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaasdf" << i << "\n\r";
    }
    strm << ch->name << " is a turkey with " << ch->hit << " health\n\r";

    std::string str = strm.str();

    send_to_char(str.c_str(), ch);


}
}