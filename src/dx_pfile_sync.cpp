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
            entry.AddVal<const char *>((&(pEnt->d_name[0])));
            entry.AddVal(statBuf.st_mtime);

            seq.AddVal<DxVal &&>( move(entry) );
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

    const DxStr &msgVal = static_cast<const DxStr &>(msg.GetMsgVal());
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
        outMsgVal.AddKeyVal( "pfile_name", pfileName.c_str() );
        outMsgVal.AddKeyVal( "pfile_val", nullptr );

        unique_ptr<DxMsg> outMsg( new DxMsg(
            DxStr("pfile"),
            move( outMsgVal )
            ));
        
        mpParent->mpDc->PushMsg( move(outMsg) );
        return;
    }

    CHAR_DATA *ch = d->character;

    DxMap pfileVal;

    pfileVal.AddKeyVal( "name", ch->name );
    pfileVal.AddKeyVal( "prompt", ch->prompt );
    pfileVal.AddKeyVal( "clan", clan_table[ch->clan].name );
    pfileVal.AddKeyVal( "sex", ( ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female"));
    pfileVal.AddKeyVal( "class", class_table[ch->clss].name );
    if ( ch->race < MAX_PC_RACE )
    {
        pfileVal.AddKeyVal( "race", pc_race_table[ch->race].name );
    }
    else
    {
        pfileVal.AddKeyVal( "race", nullptr );
    }
    pfileVal.AddKeyVal( "level", ch->level );
    pfileVal.AddKeyVal( "played", ch->played );
    pfileVal.AddKeyVal( "hit", ch->hit );
    pfileVal.AddKeyVal( "max_hit", ch->max_hit );
    pfileVal.AddKeyVal( "mana", ch->mana );
    pfileVal.AddKeyVal( "max_mana", ch->max_mana );
    pfileVal.AddKeyVal( "move", ch->move );
    pfileVal.AddKeyVal( "max_move", ch->max_move );
    pfileVal.AddKeyVal( "gold", ch->gold );
    pfileVal.AddKeyVal( "exp", ch->exp );
    pfileVal.AddKeyVal( "hitroll", ch->hitroll);
    pfileVal.AddKeyVal( "damroll", ch->damroll);
    pfileVal.AddKeyVal( "armor", ch->armor);
    pfileVal.AddKeyVal( "heavy_armor", ch->heavy_armor );

    PC_DATA *pc = ch->pcdata;

    pfileVal.AddKeyVal( "remorts", pc->remorts );
    pfileVal.AddKeyVal( "ascents", pc->ascents );


    DxMap outMsgVal;
    outMsgVal.AddKeyVal( "pfile_name", pfileName.c_str() );
    outMsgVal.AddKeyVal<DxVal &&>( "pfile_val", move( pfileVal ) );

    free_char(ch);
    free_descriptor(d);

    unique_ptr<const DxMsg> outMsg( new DxMsg(
        DxStr( "pfile" ),
        move( outMsgVal )));

    mpParent->mpDc->PushMsg( move(outMsg) );
}
