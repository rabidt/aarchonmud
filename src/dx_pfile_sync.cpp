#include "dx_pfile_sync.hpp"

extern "C" {
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "merc.h"
}


using std::unique_ptr;
using std::move;


void
DxPfileSync::ReqPfileSeqHndlr::HandleMsg( const DxMsg &msg )
{
    DIR *pDir = opendir(PLAYER_DIR);

    if (!pDir)
    {
        // TODO: bug message
        return;
    }

    DxSeq seq;

    struct dirent *pEnt;

    while (true)
    {
        errno = 0;
        pEnt = ::readdir(pDir);

        if (!pEnt)
            break;

        if ( pEnt->d_type == DT_REG )
        {
            std::string fPath(PLAYER_DIR);
            fPath += pEnt->d_name;

            struct stat statBuf;
            ::stat(fPath.c_str(), &statBuf);

            DxSeq entry;
            entry.AddVal(DxStr(&(pEnt->d_name[0])));
            entry.AddVal(DxStr(std::to_string(statBuf.st_mtime)));

            seq.AddVal( move(entry) );
        }
    }

    if (errno != 0)
    {
        // TODO: bug out
        ::closedir( pDir );
        return;
    }

    ::closedir( pDir );

    unique_ptr<const DxMsg> outMsg( new DxMsg( 
        DxStr("pfile_seq"),
        move(seq)
    )); 

    mpParent->mpDc->PushMsg( move(outMsg) );
}

void
DxPfileSync::ReqPfileHndlr::HandleMsg( const DxMsg &msg )
{
    if (msg.GetMsgVal().GetType() != DxValTypeEnum::DX_STR)
    {
        // TODO: bug message
        return;
    }

    const DxStr &msgVal = dynamic_cast<const DxStr &>(msg.GetMsgVal());
    const std::string &pfileName = msgVal.GetVal();

    DESCRIPTOR_DATA *d = new_descriptor();

    if (!load_char_obj(d, pfileName.c_str(), true))
    {
        // no pfile found, TODO: bug out
        if (d->character)
        {
            free_char(d->character);
            d->character = NULL;
        }
        free_descriptor(d);

        DxMap outMsgVal;
        outMsgVal.AddKeyVal(
            DxStr("pfile_name"),
            DxStr(std::string(pfileName)));
        outMsgVal.AddKeyVal(
            DxStr("pfile_val"),
            DxNull());

        unique_ptr<DxMsg> outMsg( new DxMsg(
            DxStr("pfile"),
            move( outMsgVal )
            ));
        
        mpParent->mpDc->PushMsg( move(outMsg) );
        return;
    }

    CHAR_DATA *ch = d->character;

    DxMap pfileVal;

    pfileVal.AddKeyVal(
        DxStr( "name" ),
        DxStr( ch->name ));
    pfileVal.AddKeyVal(
        DxStr( "level" ),
        DxInt32( ch->level ));

    DxMap outMsgVal;
    outMsgVal.AddKeyVal(
        DxStr( "pfile_name" ),
        DxStr( std::string(pfileName) ));
    outMsgVal.AddKeyVal(
        DxStr( "pfile_val" ),
        move( pfileVal ));

    free_char(ch);
    free_descriptor(d);

    unique_ptr<const DxMsg> outMsg( new DxMsg(
        DxStr( "pfile" ),
        move( outMsgVal )));

    mpParent->mpDc->PushMsg( move(outMsg) );
}
