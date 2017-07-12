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

    unique_ptr<DxValSeq> seq( new DxValSeq );

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

            unique_ptr<DxValSeq> entry( new DxValSeq );
            entry->AddVal(unique_ptr<DxVal>( 
                new DxValString( &(pEnt->d_name[0]))
                ));
            entry->AddVal(unique_ptr<DxVal>(
                new DxValString(
                    std::to_string(statBuf.st_mtime))
                ));

            seq->AddVal( move(entry) );
        }
    }

    if (errno != 0)
    {
        // TODO: bug out
        ::closedir( pDir );
        return;
    }

    ::closedir( pDir );

    unique_ptr<const DxMsg> outMsg( 
        new DxMsg(
            unique_ptr<DxValString>(new DxValString("pfile_seq")),
            move(seq)
        )
    );

    mpParent->mpDc->PushMsg( move(outMsg) );
}

void
DxPfileSync::ReqPfileHndlr::HandleMsg( const DxMsg &msg )
{
    if (msg.GetMsgVal().GetType() != DX_STRING)
    {
        // TODO: bug message
        return;
    }

    const DxValString &msgVal = dynamic_cast<const DxValString &>(msg.GetMsgVal());
    const std::string &pfileName = msgVal.GetVal();

    DESCRIPTOR_DATA *d = new_descriptor();

    if (!load_char_obj(d, pfileName.c_str(), true))
    {
        // TODO: bug out
        if (d->character)
        {
            free_char(d->character);
            d->character = NULL;
        }
        free_descriptor(d);
        return;
    }

    CHAR_DATA *ch = d->character;

    unique_ptr<DxValMap> plrMap( new DxValMap );

    plrMap->AddKeyVal(
        new DxValString("name"),
        new DxValString(ch->name)
        );

    plrMap->AddKeyVal(
        new DxValString("level"),
        new DxValInt32(ch->level));


    unique_ptr<const DxMsg> outMsg(
        new DxMsg(
            unique_ptr<const DxValString>( new DxValString("pfile")),
            move(plrMap)
        )
    );

    free_char(ch);
    free_descriptor(d);

    mpParent->mpDc->PushMsg( move(outMsg) );
}
