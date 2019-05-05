#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <sqlite3.h>

extern "C" {
#include <lauxlib.h>
#include "merc.h"
#include "db.h"
#include "lua_main.h"
}

extern "C" void db_dump_main( void );
extern "C" { extern int fingertime; }


#define ASSERTM( cond, msg ) \
    do { \
        if (!(cond)) \
        { \
            std::cerr << __func__ << "@" << __LINE__ << "::ASSERT FAIL[" << #cond << "]::" \
                      << msg << std::endl; \
            ::exit(1); \
        } \
    } while (0)

#define ASSERT( cond ) \
    ASSERTM(cond, "")

#define BINT(st, name, val) \
    do { \
        int ind_ = sqlite3_bind_parameter_index(st, name); \
        ASSERTM( ind_ != 0, name << " lookup failed."); \
        ASSERT( SQLITE_OK == sqlite3_bind_int(st, ind_, val)); \
    } while (0)

#define BINT64(st, name, val) \
    do { \
        int ind_ = sqlite3_bind_parameter_index(st, name); \
        ASSERTM( ind_ != 0, name << " lookup failed."); \
        ASSERT( SQLITE_OK == sqlite3_bind_int64(st, ind_, val)); \
    } while (0)

#define BTEXT(st, name, val) \
    do { \
        int ind_ = sqlite3_bind_parameter_index(st, name); \
        ASSERTM( ind_ != 0, name << " lookup failed."); \
        ASSERT( SQLITE_OK == sqlite3_bind_text(st, ind_, val, -1, SQLITE_STATIC)); \
    } while (0)


const char * const cDbPath = "aeaea_db.sqlite3";



class DbMgr
{
public:
    DbMgr()
        : mTbls()
    { }

    typedef size_t tblid_t;

    tblid_t NewTbl(std::string name)
    {
        mTbls.emplace_back(name);
        return mTbls.size() - 1;
    }
    void AddCol(tblid_t id, std::string colName, std::string colConstraint)
    {
        mTbls[id].mCols.emplace_back(colName, colConstraint);
    }

    void AddConstraint(tblid_t id, std::string constraint)
    {
        mTbls[id].mConstraints.emplace_back(constraint);
    }

    void Init(sqlite3 *db);
    void Cleanup(sqlite3 *db);
    sqlite3_stmt *GetInsertStmt(tblid_t id)
    {
        return mTbls[id].mStmt;
    }

private:
    struct Tbl
    {
        Tbl(std::string name)
            : mName(name)
            , mCols()
            , mConstraints()
            , mStmt(nullptr)
        { }
        Tbl(Tbl &&) = default;
        Tbl(const Tbl&) = delete;
        Tbl operator=(const Tbl&) = delete;

        std::string mName;
        std::vector<std::pair<std::string, std::string>> mCols;
        std::vector<std::string> mConstraints;
        sqlite3_stmt *mStmt;
    };

    std::vector<Tbl> mTbls;
};

void DbMgr::Init(sqlite3 *db)
{
    char *zErrMsg = NULL;

    for (auto &tbl : mTbls)
    {
        std::ostringstream oss;
        oss << "CREATE TABLE " << tbl.mName << " (\n";
        for (unsigned i = 0; i < tbl.mCols.size(); ++i)
        {
            if (i > 0)
            {
                oss << ",\n";
            }
            oss << "    " << tbl.mCols[i].first << " " << tbl.mCols[i].second;
        }

        for (auto &con : tbl.mConstraints)
        {
            oss << ",\n"
                "    " << con;
        }

        oss << ");";

        std::string str = oss.str();

        ASSERTM( SQLITE_OK == sqlite3_exec(db, str.c_str(), 0, 0, &zErrMsg), 
            "CREATE TABLE " << tbl.mName << ": " << zErrMsg);
    }

    for (auto &tbl : mTbls)
    {
        std::ostringstream oss;
        oss << "INSERT INTO " << tbl.mName << " VALUES (\n";
        for (unsigned i = 0; i < tbl.mCols.size(); ++i)
        {
            if (i > 0)
            {
                oss << ",\n";
            }
            oss << "    :" << tbl.mCols[i].first;
        }

        oss << ");";

        std::string str = oss.str();

        ASSERTM( SQLITE_OK == sqlite3_prepare_v2(db, str.c_str(), -1, &tbl.mStmt, NULL), 
            sqlite3_errmsg(db));
    }
}

void DbMgr::Cleanup(sqlite3 *db)
{
    for (auto &tbl : mTbls)
    {
        ASSERTM( SQLITE_OK == sqlite3_finalize(tbl.mStmt), sqlite3_errmsg(db));
    }
}


static DbMgr sDbMgr;

static DbMgr::tblid_t ID_areas;
static DbMgr::tblid_t ID_area_flags;
static DbMgr::tblid_t ID_mobs;
static DbMgr::tblid_t ID_objs;
static DbMgr::tblid_t ID_rooms;
static DbMgr::tblid_t ID_room_flags;
static DbMgr::tblid_t ID_exits;
static DbMgr::tblid_t ID_exit_flags;
static DbMgr::tblid_t ID_resets;
static DbMgr::tblid_t ID_players;
static DbMgr::tblid_t ID_player_skills;
static DbMgr::tblid_t ID_player_masteries;
static DbMgr::tblid_t ID_player_objs;
static DbMgr::tblid_t ID_player_aliases;
static DbMgr::tblid_t ID_player_qstats;
static DbMgr::tblid_t ID_player_tattoos;
static DbMgr::tblid_t ID_player_crimes;
static DbMgr::tblid_t ID_player_stats;
static DbMgr::tblid_t ID_player_invites;
static DbMgr::tblid_t ID_box_objs;
static DbMgr::tblid_t ID_notes;
static DbMgr::tblid_t ID_notes_to;
static DbMgr::tblid_t ID_changelog;


static void create_tables(sqlite3 *db)
{

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("areas");
        sDbMgr.AddCol(id, "vnum", "INTEGER PRIMARY KEY");
        sDbMgr.AddCol(id, "name", "TEXT");
        sDbMgr.AddCol(id, "file_name", "TEXT");
        sDbMgr.AddCol(id, "credits", "TEXT");
        sDbMgr.AddCol(id, "comments", "TEXT");
        sDbMgr.AddCol(id, "min_vnum", "INTEGER");
        sDbMgr.AddCol(id, "max_vnum", "INTEGER");
        sDbMgr.AddCol(id, "security", "INTEGER");
        sDbMgr.AddCol(id, "minlevel", "INTEGER");
        sDbMgr.AddCol(id, "maxlevel", "INTEGER");

        ID_areas = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("area_flags");
        sDbMgr.AddCol(id, "area_vnum", "INTEGER");
        for (int i = 0; area_flags[i].name; ++i)
        {
            sDbMgr.AddCol(id, area_flags[i].name, "BOOLEAN");
        }
        sDbMgr.AddConstraint(id, "FOREIGN KEY(area_vnum) REFERENCES areas(vnum)");

        ID_area_flags = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("rooms");
        sDbMgr.AddCol(id, "area_vnum",  "INTEGER");
        sDbMgr.AddCol(id, "vnum",  "INTEGER");
        sDbMgr.AddCol(id, "name",  "TEXT");
        sDbMgr.AddCol(id, "description",  "TEXT");
        sDbMgr.AddCol(id, "comments",  "TEXT");
        sDbMgr.AddCol(id, "owner",  "TEXT");
        sDbMgr.AddCol(id, "light",  "INTEGER");
        sDbMgr.AddCol(id, "sector_type",  "TEXT");
        sDbMgr.AddCol(id, "heal_rate",  "INTEGER");
        sDbMgr.AddCol(id, "mana_rate",  "INTEGER");
        sDbMgr.AddCol(id, "clan",  "TEXT");
        sDbMgr.AddCol(id, "clan_rank",  "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(area_vnum) REFERENCES areas(vnum)");

        ID_rooms = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("room_flags");
        sDbMgr.AddCol(id, "room_vnum", "INTEGER");
        for (int i = 0; room_flags[i].name; ++i)
        {
            sDbMgr.AddCol(id, room_flags[i].name, "BOOLEAN");
        }
        sDbMgr.AddConstraint(id, "FOREIGN KEY(room_vnum) REFERENCES rooms(vnum)");

        ID_room_flags = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("exits");
        sDbMgr.AddCol(id,"room_vnum", "INTEGER");
        sDbMgr.AddCol(id,"to_room_vnum", "INTEGER");
        sDbMgr.AddCol(id,"dir", "TEXT");
        sDbMgr.AddCol(id,"key", "INTEGER");
        sDbMgr.AddCol(id,"keyword", "TEXT");
        sDbMgr.AddCol(id,"description", " TEXT");
        sDbMgr.AddConstraint(id,"PRIMARY KEY(room_vnum, dir)");
        sDbMgr.AddConstraint(id,"FOREIGN KEY(room_vnum) REFERENCES rooms(vnum)");
        sDbMgr.AddConstraint(id,"FOREIGN KEY(to_room_vnum) REFERENCES rooms(vnum)");

        ID_exits = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("exit_flags");
        sDbMgr.AddCol(id, "room_vnum", "INTEGER");
        sDbMgr.AddCol(id, "dir", "TEXT");
        for (int i = 0; exit_flags[i].name; ++i)
        {
            sDbMgr.AddCol(id, exit_flags[i].name, "BOOLEAN");
        }
        sDbMgr.AddConstraint(id, "FOREIGN KEY(room_vnum, dir) REFERENCES exits(room_vnum, dir)");

        ID_exit_flags = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("resets");
        sDbMgr.AddCol(id, "room_vnum", "INTEGER");
        sDbMgr.AddCol(id, "command", "TEXT");
        sDbMgr.AddCol(id, "arg1", "INTEGER");
        sDbMgr.AddCol(id, "arg2", "INTEGER");
        sDbMgr.AddCol(id, "arg3", "INTEGER");
        sDbMgr.AddCol(id, "arg4", "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(room_vnum) REFERENCES rooms(vnum)");

        ID_resets = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("mobs");
        sDbMgr.AddCol(id, "area_vnum", "INTEGER");
        sDbMgr.AddCol(id, "vnum", "INTEGER PRIMARY KEY");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "level", "INTEGER");
        sDbMgr.AddCol(id, "short_descr", "TEXT");
        sDbMgr.AddCol(id, "long_descr", "TEXT");
        sDbMgr.AddCol(id, "description", "TEXT");
        sDbMgr.AddCol(id, "comments", "TEXT");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(area_vnum) REFERENCES areas(vnum)");

        ID_mobs = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("objs");
        sDbMgr.AddCol(id, "area_vnum", "INTEGER");
        sDbMgr.AddCol(id, "vnum", "INTEGER PRIMARY KEY");
        sDbMgr.AddCol(id, "name", "TEXT");
        sDbMgr.AddCol(id, "short_descr", "TEXT");
        sDbMgr.AddCol(id, "description", "TEXT");
        sDbMgr.AddCol(id, "comments", "TEXT");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(area_vnum) REFERENCES areas(vnum)");

        ID_objs = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("players");
        sDbMgr.AddCol(id, "name", "TEXT PRIMARY KEY");
        sDbMgr.AddCol(id, "pwd", "TEXT");
        sDbMgr.AddCol(id, "level", "INTEGER");
        sDbMgr.AddCol(id, "sex", "TEXT");
        sDbMgr.AddCol(id, "class", "TEXT");
        sDbMgr.AddCol(id, "race", "TEXT");
        sDbMgr.AddCol(id, "clan", "TEXT");
        sDbMgr.AddCol(id, "clan_rank", "INTEGER");
        sDbMgr.AddCol(id, "behead_cnt", "INTEGER");
        sDbMgr.AddCol(id, "mob_kills", "INTEGER");
        sDbMgr.AddCol(id, "mob_deaths", "INTEGER");
        sDbMgr.AddCol(id, "quest_failed", "INTEGER");
        sDbMgr.AddCol(id, "quest_success", "INTEGER");
        sDbMgr.AddCol(id, "quest_hard_success", "INTEGER");
        sDbMgr.AddCol(id, "quest_hard_failed", "INTEGER");
        sDbMgr.AddCol(id, "pkill_count", "INTEGER");
        sDbMgr.AddCol(id, "remorts", "INTEGER");
        sDbMgr.AddCol(id, "armageddon_won", "INTEGER");
        sDbMgr.AddCol(id, "armageddon_lost", "INTEGER");
        sDbMgr.AddCol(id, "armageddon_kills", "INTEGER");
        sDbMgr.AddCol(id, "clan_won", "INTEGER");
        sDbMgr.AddCol(id, "clan_lost", "INTEGER");
        sDbMgr.AddCol(id, "clan_kills", "INTEGER");
        sDbMgr.AddCol(id, "race_won", "INTEGER");
        sDbMgr.AddCol(id, "race_lost", "INTEGER");
        sDbMgr.AddCol(id, "race_kills", "INTEGER");
        sDbMgr.AddCol(id, "class_won", "INTEGER");
        sDbMgr.AddCol(id, "class_lost", "INTEGER");
        sDbMgr.AddCol(id, "class_kills", "INTEGER");
        sDbMgr.AddCol(id, "warpoints", "INTEGER");
        sDbMgr.AddCol(id, "war_kills", "INTEGER");
        sDbMgr.AddCol(id, "gender_kills", "INTEGER");
        sDbMgr.AddCol(id, "gender_won", "INTEGER");
        sDbMgr.AddCol(id, "gender_lost", "INTEGER");
        sDbMgr.AddCol(id, "religion_kills", "INTEGER");
        sDbMgr.AddCol(id, "religion_won", "INTEGER");
        sDbMgr.AddCol(id, "religion_lost", "INTEGER");
        sDbMgr.AddCol(id, "duel_kills", "INTEGER");
        sDbMgr.AddCol(id, "duel_won", "INTEGER");
        sDbMgr.AddCol(id, "duel_lost", "INTEGER");
        sDbMgr.AddCol(id, "id", "INTEGER");
        sDbMgr.AddCol(id, "gold", "INTEGER");
        sDbMgr.AddCol(id, "silver", "INTEGER");
        sDbMgr.AddCol(id, "bank", "INTEGER");
        sDbMgr.AddCol(id, "storage_boxes", "INTEGER");
        sDbMgr.AddCol(id, "lastlogoff", "INTEGER");
        sDbMgr.AddCol(id, "questpoints", "INTEGER");
        sDbMgr.AddCol(id, "achpoints", "INTEGER");
        sDbMgr.AddCol(id, "explored", "INTEGER");
        sDbMgr.AddCol(id, "last_host", "TEXT");
        sDbMgr.AddCol(id, "authed_by", "TEXT");
        sDbMgr.AddCol(id, "played", "INTEGER");
        sDbMgr.AddCol(id, "hit", "INTEGER");
        sDbMgr.AddCol(id, "max_hit", "INTEGER");
        sDbMgr.AddCol(id, "mana", "INTEGER");
        sDbMgr.AddCol(id, "max_mana", "INTEGER");
        sDbMgr.AddCol(id, "move", "INTEGER");
        sDbMgr.AddCol(id, "max_move", "INTEGER");
        sDbMgr.AddCol(id, "trained_hit", "INTEGER");
        sDbMgr.AddCol(id, "trained_mana", "INTEGER");
        sDbMgr.AddCol(id, "trained_move", "INTEGER");
        sDbMgr.AddCol(id, "ascents", "INTEGER");
        sDbMgr.AddCol(id, "subclass", "TEXT");
        sDbMgr.AddCol(id, "subclass2", "TEXT");
        sDbMgr.AddCol(id, "god_name", "TEXT");
        sDbMgr.AddCol(id, "prompt", "TEXT");

        ID_players = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_skills");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "skill_name", "TEXT");
        sDbMgr.AddCol(id, "percent", "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_skills = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_masteries");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "skill_name", "TEXT");
        sDbMgr.AddCol(id, "level", "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_masteries = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_objs");
        sDbMgr.AddCol(id, "player_name",  "TEXT");
        sDbMgr.AddCol(id, "vnum",  "INTEGER");
        sDbMgr.AddCol(id, "owner",  "TEXT");
        sDbMgr.AddCol(id, "name",  "TEXT");
        sDbMgr.AddCol(id, "short_descr",  "TEXT");
        sDbMgr.AddCol(id, "description",  "TEXT");
        sDbMgr.AddCol(id, "material",  "TEXT");
        sDbMgr.AddCol(id, "wear_loc",  "TEXT");
        sDbMgr.AddCol(id, "level",  "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(vnum) REFERENCES objs(vnum)");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_objs = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("box_objs");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "box_num", "INTEGER");
        sDbMgr.AddCol(id, "vnum",  "INTEGER");
        sDbMgr.AddCol(id, "owner",  "TEXT");
        sDbMgr.AddCol(id, "name",  "TEXT");
        sDbMgr.AddCol(id, "short_descr",  "TEXT");
        sDbMgr.AddCol(id, "description",  "TEXT");
        sDbMgr.AddCol(id, "material",  "TEXT");
        sDbMgr.AddCol(id, "level",  "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(vnum) REFERENCES objs(vnum)");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_box_objs = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_aliases");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "alias", "TEXT");
        sDbMgr.AddCol(id, "command", "TEXT");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_aliases = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_qstats");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "id", "INTEGER");
        sDbMgr.AddCol(id, "status", "INTEGER");
        sDbMgr.AddCol(id, "timer", "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_qstats = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_tattoos");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "location", "TEXT");
        sDbMgr.AddCol(id, "tattoo", "TEXT");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_tattoos = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_crimes");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "crime_name", "TEXT");
        sDbMgr.AddCol(id, "imm_name", "TEXT");
        sDbMgr.AddCol(id, "timestamp", "INTEGER");
        sDbMgr.AddCol(id, "forgive", "BOOLEAN");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_crimes = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_stats");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "type", "TEXT");
        sDbMgr.AddCol(id, "str", "INTEGER");
        sDbMgr.AddCol(id, "con", "INTEGER");
        sDbMgr.AddCol(id, "vit", "INTEGER");
        sDbMgr.AddCol(id, "agi", "INTEGER");
        sDbMgr.AddCol(id, "dex", "INTEGER");
        sDbMgr.AddCol(id, "int", "INTEGER");
        sDbMgr.AddCol(id, "wis", "INTEGER");
        sDbMgr.AddCol(id, "dis", "INTEGER");
        sDbMgr.AddCol(id, "cha", "INTEGER");
        sDbMgr.AddCol(id, "luc", "INTEGER");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");

        ID_player_stats = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("player_invites");
        sDbMgr.AddCol(id, "player_name", "TEXT");
        sDbMgr.AddCol(id, "clan_name", "TEXT");
        sDbMgr.AddCol(id, "inviter", "TEXT");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(player_name) REFERENCES players(name)");
        
        ID_player_invites = id;   
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("notes");
        sDbMgr.AddCol(id, "board", "TEXT");
        sDbMgr.AddCol(id, "sender", "TEXT");
        sDbMgr.AddCol(id, "date", "TEXT");
        sDbMgr.AddCol(id, "subject", "TEXT");
        sDbMgr.AddCol(id, "text", "TEXT");
        sDbMgr.AddCol(id, "date_stamp", "INTEGER");
        sDbMgr.AddCol(id, "expire", "INTEGER");
        sDbMgr.AddConstraint(id, "PRIMARY KEY(sender, date_stamp)");

        ID_notes = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("notes_to");
        sDbMgr.AddCol(id, "note_id", "INTEGER");
        sDbMgr.AddCol(id, "to_name", "TEXT");
        sDbMgr.AddConstraint(id, "FOREIGN KEY(note_id) REFERENCES notes(rowid)");

        ID_notes_to = id;
    }

    {
        DbMgr::tblid_t id = sDbMgr.NewTbl("changelog");
        sDbMgr.AddCol(id, "date", "INTEGER");
        sDbMgr.AddCol(id, "author", "TEXT");
        sDbMgr.AddCol(id, "desc", "TEXT");

        ID_changelog = id;
    }
}

static void dump_area_flags(sqlite3 *db, AREA_DATA *pArea)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_area_flags);

    ASSERT( SQLITE_OK == sqlite3_reset(st));
    ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

    BINT(st, ":area_vnum", pArea->vnum);
    for (int i = 0; area_flags[i].name; ++i)
    {
        std::string param = std::string(":") + std::string(area_flags[i].name);
        BINT(st, param.c_str(), IS_SET(pArea->area_flags, area_flags[i].bit));
    }

    ASSERT( SQLITE_DONE == sqlite3_step(st) );
}

static void dump_areas(sqlite3 *db)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_areas);

    for (AREA_DATA *pArea = area_first; pArea; pArea = pArea->next)
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BINT( st, ":vnum", pArea->vnum);
        BTEXT(st, ":name", pArea->name);
        BTEXT(st, ":file_name", pArea->file_name);
        BTEXT(st, ":credits", pArea->credits);
        BTEXT(st, ":comments", pArea->comments);
        BINT( st, ":min_vnum", pArea->min_vnum);
        BINT( st, ":max_vnum", pArea->max_vnum);
        BINT( st, ":security", pArea->security);
        BINT( st, ":minlevel", pArea->minlevel);
        BINT( st, ":maxlevel", pArea->maxlevel);

        dump_area_flags(db, pArea);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }
}

static void dump_mobs(sqlite3 *db)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_mobs);

    for( int iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( MOB_INDEX_DATA *pMob = mob_index_hash[iHash]; pMob; pMob = pMob->next )
        {
            ASSERT( SQLITE_OK == sqlite3_reset(st));
            ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

            BINT( st, ":area_vnum", pMob->area->vnum);
            BINT( st, ":vnum", pMob->vnum);
            BTEXT(st, ":player_name", pMob->player_name);
            BINT( st, ":level", pMob->level);
            BTEXT(st, ":short_descr", pMob->short_descr);
            BTEXT(st, ":long_descr", pMob->long_descr);
            BTEXT(st, ":description", pMob->description);
            BTEXT(st, ":comments", pMob->comments);

            ASSERT( SQLITE_DONE == sqlite3_step(st) );
        }
    }
}

static void dump_objs(sqlite3 *db)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_objs);

    for( int iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( OBJ_INDEX_DATA *pObj = obj_index_hash[iHash]; pObj; pObj = pObj->next )
        {
            ASSERT( SQLITE_OK == sqlite3_reset(st));
            ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

            BINT( st, ":area_vnum", pObj->area->vnum);
            BINT( st, ":vnum", pObj->vnum);
            BTEXT(st, ":name", pObj->name);
            BTEXT(st, ":short_descr", pObj->short_descr);
            BTEXT(st, ":description", pObj->description);
            BTEXT(st, ":comments", pObj->comments);

            ASSERT( SQLITE_DONE == sqlite3_step(st) );
        }
    }
}

static void dump_room_flags(sqlite3 *db, ROOM_INDEX_DATA *pRoom)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_room_flags);
    
    ASSERT( SQLITE_OK == sqlite3_reset(st));
    ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

    BINT(st, ":room_vnum", pRoom->vnum);
    for (int i = 0; room_flags[i].name; ++i)
    {
        std::string param = std::string(":") + std::string(room_flags[i].name);
        BINT(st, param.c_str(), IS_SET(pRoom->room_flags, room_flags[i].bit));
    }

    ASSERT( SQLITE_DONE == sqlite3_step(st) );
}

static void dump_exit_flags(sqlite3 *db, ROOM_INDEX_DATA *pRoom, int door, EXIT_DATA *ex)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_exit_flags);

    ASSERT( SQLITE_OK == sqlite3_reset(st));
    ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

    BINT(st, ":room_vnum", pRoom->vnum);
    BTEXT(st, ":dir", dir_name[door]);
    for (int i = 0; exit_flags[i].name; ++i)
    {
        std::string param = std::string(":") + std::string(exit_flags[i].name);
        BINT(st, param.c_str(), IS_SET(ex->exit_info, exit_flags[i].bit));
    }

    ASSERT( SQLITE_DONE == sqlite3_step(st) );
}

static void dump_exits(sqlite3 *db, ROOM_INDEX_DATA *pRoom)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_exits);

    for (int door = 0; door < MAX_DIR; ++door)
    {
        EXIT_DATA *ex = pRoom->exit[door];
        if (!ex)
        {
            continue;
        }

        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BINT(st, ":room_vnum", pRoom->vnum);
        if (ex->u1.to_room)
        {
            BINT(st, ":to_room_vnum", ex->u1.to_room->vnum);
        }
        BTEXT(st, ":dir", dir_name[door]);
        BINT(st, ":key", ex->key);
        BTEXT(st, ":keyword", ex->keyword);
        BTEXT(st, ":description", ex->description);

        dump_exit_flags(db, pRoom, door, ex);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }

}

static void dump_resets(sqlite3 *db, ROOM_INDEX_DATA *pRoom)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_resets);

    for (RESET_DATA *rs = pRoom->reset_first; rs; rs = rs->next)
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        char cmd[2];
        cmd[0] = rs->command;
        cmd[1] = '\0';

        BINT( st, ":room_vnum", pRoom->vnum);
        BTEXT(st, ":command", cmd);
        BINT( st, ":arg1", rs->arg1);
        BINT( st, ":arg2", rs->arg2);
        BINT( st, ":arg3", rs->arg3);
        BINT( st, ":arg4", rs->arg4);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }
}

static void dump_rooms(sqlite3 *db)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_rooms);

    for (AREA_DATA *pArea = area_first; pArea; pArea = pArea->next)
    {
        for (int vnum = pArea->min_vnum; vnum <= pArea->max_vnum; ++vnum)
        {
            ROOM_INDEX_DATA *pRoom = get_room_index(vnum);
            
            if (!pRoom)
            {
                continue;
            }

            ASSERT( SQLITE_OK == sqlite3_reset(st));
            ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

            BINT( st, ":area_vnum", pArea->vnum);
            BINT( st, ":vnum", pRoom->vnum);
            BTEXT(st, ":name", pRoom->name);
            BTEXT(st, ":description", pRoom->description);
            BTEXT(st, ":comments", pRoom->comments);
            BTEXT(st, ":owner", pRoom->owner);
            BINT( st, ":light", pRoom->light);
            BTEXT(st, ":sector_type", flag_bit_name(sector_flags, pRoom->sector_type));
            BINT( st, ":heal_rate", pRoom->heal_rate);
            BINT( st, ":mana_rate", pRoom->mana_rate);
            BTEXT(st, ":clan", clan_table[pRoom->clan].name);
            BINT( st, ":clan_rank", pRoom->clan_rank);

            ASSERT( SQLITE_DONE == sqlite3_step(st) );

            dump_room_flags(db, pRoom);
            dump_exits(db, pRoom);
            dump_resets(db, pRoom);
        }
    }
}

static void dump_player_masteries(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_masteries);

    for (int sn = 0; sn < MAX_SKILL; ++sn)
    {
        if (skill_table[sn].name && 
            ch->pcdata->learned[sn] > 0 &&
            ch->pcdata->mastered[sn] > 0)
        {
            ASSERT( SQLITE_OK == sqlite3_reset(st));
            ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

            BTEXT(st, ":player_name", ch->name);
            BTEXT(st, ":skill_name", skill_table[sn].name);
            BINT( st, ":level", ch->pcdata->mastered[sn]);

            ASSERT( SQLITE_DONE == sqlite3_step(st) );
        }
    }
}

static void dump_player_skills(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_skills);

    for (int sn = 0; sn < MAX_SKILL; ++sn)
    {
        if (skill_table[sn].name && ch->pcdata->learned[sn] > 0)
        {
            ASSERT( SQLITE_OK == sqlite3_reset(st));
            ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

            BTEXT(st, ":player_name", ch->name);
            BTEXT(st, ":skill_name", skill_table[sn].name);
            BINT( st, ":percent", ch->pcdata->learned[sn]);

            ASSERT( SQLITE_DONE == sqlite3_step(st) );
        }
    }
}

static void dump_player_objs(sqlite3 *db, CHAR_DATA *ch, OBJ_DATA *head)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_objs);

    for ( OBJ_DATA *obj = head; obj; obj = obj->next_content )
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BINT( st, ":vnum", obj->pIndexData->vnum);
        if (obj->owner)
        {
            BTEXT(st, ":owner", obj->owner);
        }
        if (obj->name != obj->pIndexData->name)
        {
            BTEXT(st, ":name", obj->name);
        }
        if (obj->short_descr != obj->pIndexData->short_descr)
        {
            BTEXT(st, ":short_descr", obj->short_descr);
        }
        if (obj->description != obj->pIndexData->description)
        {
            BTEXT(st, ":description", obj->description);
        }
        if (obj->material != obj->pIndexData->material)
        {
            BTEXT(st, ":material", obj->material);
        }
        // TODO: extra flags

        BTEXT(st, ":wear_loc", flag_bit_name(wear_loc_flags, obj->wear_loc));

        if (obj->level != obj->pIndexData->level)
        {
            BINT(st, ":level", obj->level);
        }

        ASSERT( SQLITE_DONE == sqlite3_step(st) );

        if (obj->contains)
        {
            dump_player_objs(db, ch, obj->contains);
        }
    }

}

static void dump_box_contents(sqlite3 *db, CHAR_DATA *ch, int box_num, OBJ_DATA *contents)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_box_objs);

    for ( OBJ_DATA *obj = contents; obj; obj = obj->next_content )
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BINT(st, ":box_num", box_num);
        BINT( st, ":vnum", obj->pIndexData->vnum);
        if (obj->owner)
        {
            BTEXT(st, ":owner", obj->owner);
        }
        if (obj->name != obj->pIndexData->name)
        {
            BTEXT(st, ":name", obj->name);
        }
        if (obj->short_descr != obj->pIndexData->short_descr)
        {
            BTEXT(st, ":short_descr", obj->short_descr);
        }
        if (obj->description != obj->pIndexData->description)
        {
            BTEXT(st, ":description", obj->description);
        }
        if (obj->material != obj->pIndexData->material)
        {
            BTEXT(st, ":material", obj->material);
        }
        // TODO: extra flags

        if (obj->level != obj->pIndexData->level)
        {
            BINT(st, ":level", obj->level);
        }

        ASSERT( SQLITE_DONE == sqlite3_step(st) );

        if (obj->contains)
        {
            dump_box_contents(db, ch, box_num, obj->contains);
        }
    }
}

static void dump_player_boxes(sqlite3 *db, CHAR_DATA *ch)
{
    if (ch->pcdata->storage_boxes < 1)
    {
        return;
    }

    char filepath[MAX_INPUT_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%s_box", BOX_DIR, capitalize(ch->name));

    struct stat buffer;
    if (0 != ::stat(filepath, &buffer))
    {
        // No box file
        return;
    }
    ASSERT( true == load_storage_boxes(ch) );

    for (int i = 0; i < ch->pcdata->storage_boxes; ++i)
    {
        ASSERT( NULL != ch->pcdata->box_data[i] );
        dump_box_contents(db, ch, i, ch->pcdata->box_data[i]->contains);
    }
}

static void dump_player(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_players);

    ASSERT( SQLITE_OK == sqlite3_reset(st));
    ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

    BTEXT(st, ":name", ch->name);
    BTEXT(st, ":pwd", ch->pcdata->pwd);
    BINT( st, ":level", ch->level);
    BTEXT(st, ":sex", sex_table[ch->sex].name);
    BTEXT(st, ":class", class_table[ch->clss].name);
    BTEXT(st, ":race", race_table[ch->race].name);
    BTEXT(st, ":clan", clan_table[ch->clan].name);
    BINT( st, ":clan_rank", ch->pcdata->clan_rank);
    BINT( st, ":behead_cnt", ch->pcdata->behead_cnt);
    BINT( st, ":mob_kills", ch->pcdata->mob_kills);
    BINT( st, ":mob_deaths", ch->pcdata->mob_deaths);
    BINT( st, ":quest_failed", ch->pcdata->quest_failed);
    BINT( st, ":quest_success", ch->pcdata->quest_success);
    BINT( st, ":quest_hard_success", ch->pcdata->quest_hard_success);
    BINT( st, ":quest_hard_failed", ch->pcdata->quest_hard_failed);
    BINT( st, ":pkill_count", ch->pcdata->pkill_count);
    BINT( st, ":remorts", ch->pcdata->remorts);
    BINT( st, ":armageddon_won", ch->pcdata->armageddon_won);
    BINT( st, ":armageddon_lost", ch->pcdata->armageddon_lost);
    BINT( st, ":armageddon_kills", ch->pcdata->armageddon_kills);
    BINT( st, ":clan_won", ch->pcdata->clan_won);
    BINT( st, ":clan_lost", ch->pcdata->clan_lost);
    BINT( st, ":clan_kills", ch->pcdata->clan_kills);
    BINT( st, ":race_won", ch->pcdata->race_won);
    BINT( st, ":race_lost", ch->pcdata->race_lost);
    BINT( st, ":race_kills", ch->pcdata->race_kills);
    BINT( st, ":class_won", ch->pcdata->class_won);
    BINT( st, ":class_lost", ch->pcdata->class_lost);
    BINT( st, ":class_kills", ch->pcdata->class_kills);
    BINT( st, ":warpoints", ch->pcdata->warpoints);
    BINT( st, ":war_kills", ch->pcdata->war_kills);
    BINT( st, ":gender_kills", ch->pcdata->gender_kills);
    BINT( st, ":gender_won", ch->pcdata->gender_won);
    BINT( st, ":gender_lost", ch->pcdata->gender_lost);
    BINT( st, ":religion_kills", ch->pcdata->religion_kills);
    BINT( st, ":religion_won", ch->pcdata->religion_won);
    BINT( st, ":religion_lost", ch->pcdata->religion_lost);
    BINT( st, ":duel_kills", ch->pcdata->duel_kills);
    BINT( st, ":duel_won", ch->pcdata->duel_won);
    BINT( st, ":duel_lost", ch->pcdata->duel_lost);
    BINT( st, ":id", ch->id);
    BINT( st, ":gold", ch->gold);
    BINT( st, ":silver", ch->silver);
    BINT( st, ":bank", ch->pcdata->bank);
    BINT( st, ":storage_boxes", ch->pcdata->storage_boxes);
    BINT( st, ":lastlogoff", fingertime);
    BINT( st, ":questpoints", ch->pcdata->questpoints);
    BINT( st, ":achpoints", ch->pcdata->achpoints);
    BINT( st, ":explored", ch->pcdata->explored->set);
    BTEXT(st, ":last_host", ch->pcdata->last_host);
    BTEXT(st, ":authed_by", ch->pcdata->authed_by);
    BINT( st, ":played", ch->played);
    BINT( st, ":hit", ch->hit);
    BINT( st, ":max_hit", ch->max_hit);
    BINT( st, ":mana", ch->mana);
    BINT( st, ":max_mana", ch->max_mana);
    BINT( st, ":move", ch->move);
    BINT( st, ":max_move", ch->max_move);
    BINT( st, ":trained_hit", ch->pcdata->trained_hit);
    BINT( st, ":trained_mana", ch->pcdata->trained_mana);
    BINT( st, ":trained_move", ch->pcdata->trained_move);
    BINT( st, ":ascents", ch->pcdata->ascents);
    BTEXT(st, ":subclass", subclass_table[ch->pcdata->subclass].name);
    BTEXT(st, ":subclass2", subclass_table[ch->pcdata->subclass2].name);
    BTEXT(st, ":god_name", ch->pcdata->god_name);
    BTEXT(st, ":prompt", ch->prompt);

    ASSERT( SQLITE_DONE == sqlite3_step(st) );
}

static void dump_player_aliases(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_aliases);

    for (int i = 0; i < MAX_ALIAS; ++i)
    {
        if (ch->pcdata->alias[i] == NULL ||
            ch->pcdata->alias_sub[i] == NULL)
        {
            break;
        }

        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BTEXT(st, ":alias", ch->pcdata->alias[i]);
        BTEXT(st, ":command", ch->pcdata->alias_sub[i]);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }

}

static void dump_player_qstats(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_qstats);

    for (QUEST_DATA *qstat = ch->pcdata->qdata; qstat; qstat = qstat->next)
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BINT( st, ":id", qstat->id);
        BINT( st, ":status", qstat->status);
        BINT( st, ":timer", qstat->timer);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }
}

static void dump_player_tattoos(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_tattoos);

    for (int i = 1; i < MAX_WEAR; ++i)
    {
        if (ch->pcdata->tattoos[i] == TATTOO_NONE)
        {
            continue;
        }

        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BTEXT(st, ":location", flag_bit_name(wear_loc_flags, i));
        BTEXT(st, ":tattoo", tattoo_desc(ch->pcdata->tattoos[i]));

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }
}

static void dump_player_crimes(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_crimes);

    for (CRIME_DATA *crime = ch->pcdata->crimes; crime; crime = crime->next)
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BTEXT(st, ":crime_name", crime->name);
        BINT( st, ":imm_name", crime->timestamp);
        BINT( st, ":forgive", crime->forgive);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }
}

static void dump_player_stats(sqlite3 *db, CHAR_DATA *ch)
{
    sh_int curr_stat[MAX_STATS];

    curr_stat[STAT_STR] = get_curr_stat(ch, STAT_STR);
    curr_stat[STAT_CON] = get_curr_stat(ch, STAT_CON);
    curr_stat[STAT_VIT] = get_curr_stat(ch, STAT_VIT);
    curr_stat[STAT_AGI] = get_curr_stat(ch, STAT_AGI);
    curr_stat[STAT_DEX] = get_curr_stat(ch, STAT_DEX);
    curr_stat[STAT_INT] = get_curr_stat(ch, STAT_INT);
    curr_stat[STAT_WIS] = get_curr_stat(ch, STAT_WIS);
    curr_stat[STAT_DIS] = get_curr_stat(ch, STAT_DIS);
    curr_stat[STAT_CHA] = get_curr_stat(ch, STAT_CHA);
    curr_stat[STAT_LUC] = get_curr_stat(ch, STAT_LUC);

    struct
    {
        const char *typeName;
        sh_int *stats;
    } const pairs[] = 
    {
        { "perm_stat", ch->perm_stat },
        { "mod_stat", ch->mod_stat },
        { "original_stats", ch->pcdata->original_stats },
        { "history_stats", ch->pcdata->history_stats },
        { "curr_stat", curr_stat }
    };

    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_stats);

    for (unsigned iPair = 0; iPair < (sizeof(pairs) / sizeof(pairs[0])); ++iPair)
    {
        sh_int *stats = pairs[iPair].stats;

        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BTEXT(st, ":type", pairs[iPair].typeName);
        BINT( st, ":str", stats[STAT_STR]);
        BINT( st, ":con", stats[STAT_CON]);
        BINT( st, ":vit", stats[STAT_VIT]);
        BINT( st, ":agi", stats[STAT_AGI]);
        BINT( st, ":dex", stats[STAT_DEX]);
        BINT( st, ":int", stats[STAT_INT]);
        BINT( st, ":wis", stats[STAT_WIS]);
        BINT( st, ":dis", stats[STAT_DIS]);
        BINT( st, ":cha", stats[STAT_CHA]);
        BINT( st, ":luc", stats[STAT_LUC]);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );

    }
}

static void dump_player_invites(sqlite3 *db, CHAR_DATA *ch)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_player_invites);

    for (int i = 1; i < MAX_CLAN; ++i)
    {
        if (ch->pcdata->invitation[i] == NULL)
        {
            continue;
        }

        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":player_name", ch->name);
        BTEXT(st, ":clan_name", clan_table[i].name);
        BTEXT(st, ":inviter", ch->pcdata->invitation[i]);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );
    }   
}

static void dump_players(sqlite3 *db)
{
    struct dirent *ent;

    DIR *dir = ::opendir(PLAYER_DIR);

    ASSERT( dir );

    while ((ent = readdir(dir)) != NULL)
    {
        DESCRIPTOR_DATA *desc = new_descriptor();

        if ( !load_char_obj(desc, ent->d_name, false) )
        {
            // TODO: 
            std::cout << "COULDN'T LOAD " << ent->d_name << std::endl;

            if (desc->character)
            {
                free_char(desc->character);
                desc->character = NULL;
            }
            free_descriptor(desc);

            continue;
        }

        dump_player(db, desc->character);
        dump_player_skills(db, desc->character);
        dump_player_masteries(db, desc->character);
        dump_player_objs(db, desc->character, desc->character->carrying);
        dump_player_boxes(db, desc->character);
        dump_player_aliases(db, desc->character);
        dump_player_qstats(db, desc->character);
        dump_player_tattoos(db, desc->character);
        dump_player_crimes(db, desc->character);
        dump_player_stats(db, desc->character);
        dump_player_invites(db, desc->character);

        nuke_pets(desc->character);
        free_char(desc->character);
        free_descriptor(desc);
    }

    ::closedir(dir);
}

static void dump_board(sqlite3 *db, BOARD_DATA *board)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_notes);
    sqlite3_stmt *st_to = sDbMgr.GetInsertStmt(ID_notes_to);

    for (NOTE_DATA *note = board->note_first; note; note = note->next)
    {
        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        BTEXT(st, ":board", board->short_name);
        BTEXT(st, ":sender", note->sender);
        BTEXT(st, ":date", note->date);
        BTEXT(st, ":subject", note->subject);
        BTEXT(st, ":text", note->text);
        BINT (st, ":date_stamp", note->date_stamp);
        BINT (st, ":expire", note->expire);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );

        sqlite3_int64 note_id = sqlite3_last_insert_rowid(db);

        {
            const char *namelist = note->to_list;
            char name[MIL];

            while (true)
            {
                namelist = one_argument(namelist, name);
                if (name[0] == '\0')
                    break;

                ASSERT( SQLITE_OK == sqlite3_reset(st_to));
                ASSERT( SQLITE_OK == sqlite3_clear_bindings(st_to));

                BINT64(st_to, ":note_id", note_id);
                BTEXT(st_to, ":to_name", name);

                ASSERT( SQLITE_DONE == sqlite3_step(st_to) );
            }
        }
    }
}

static void dump_old_notes(sqlite3 *db)
{
    struct dirent *ent;

    DIR *dir = ::opendir(NOTE_DIR);

    ASSERT( dir );

    while ((ent = readdir(dir)) != NULL)
    {
        int len = strlen(ent->d_name);
        if (len >= 4 && !strcmp(ent->d_name + len - 4, ".old"))
        {
            BOARD_DATA board =
            {
                ent->d_name,
                nullptr,
                0,
                0,
                0,
                nullptr,
                0,
                0,
                0,
                nullptr,
                false
            };

            load_board(&board, true);
            dump_board(db, &board);
            // TODO: should free all the notes
        }
    }
}

static void dump_notes(sqlite3 *db)
{
    for (int i = 0; i < MAX_BOARD; ++i)
    {
        dump_board(db, &boards[i]);
    }
}

static void dump_changelog(sqlite3 *db)
{
    sqlite3_stmt *st = sDbMgr.GetInsertStmt(ID_changelog);

    lua_State *LS = g_mud_LS;

    lua_getglobal(LS, "changelog_table");

    for (int i = 1; ; ++i)
    {
        lua_rawgeti(LS, -1, i);
        if (lua_isnil(LS, -1))
        {
            lua_pop(LS, 1);
            break;
        }

        ASSERT( SQLITE_OK == sqlite3_reset(st));
        ASSERT( SQLITE_OK == sqlite3_clear_bindings(st));

        lua_getfield(LS, -1, "date");
        BINT(st, ":date", luaL_checkinteger(LS, -1));
        lua_pop(LS, 1);

        lua_getfield(LS, -1, "author");
        BTEXT(st, ":author", luaL_checkstring(LS, -1));
        lua_pop(LS, 1);

        lua_getfield(LS, -1, "desc");
        BTEXT(st, ":desc", luaL_checkstring(LS, -1));
        lua_pop(LS, 1);

        ASSERT( SQLITE_DONE == sqlite3_step(st) );

        lua_pop(LS, 1);
    }
}

void db_dump_main( void )
{
    sqlite3 *db;
    char *zErrMsg = NULL;
    
    {
        struct stat st;
        if (::stat(cDbPath, &st) == 0) {
            ::unlink(cDbPath);
        }
    }

    ASSERT( SQLITE_OK == sqlite3_open(cDbPath, &db));

    ASSERTM( SQLITE_OK == sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &zErrMsg), zErrMsg);

    create_tables(db);
    sDbMgr.Init(db);

    dump_areas(db);
    dump_rooms(db);
    dump_mobs(db);
    dump_objs(db);
    dump_players(db);
    dump_notes(db);
    dump_old_notes(db);
    dump_changelog(db);

    ASSERT( SQLITE_OK == sqlite3_exec(db, "END TRANSACTION;", 0, 0, &zErrMsg));

    // statements::cleanup();
    sDbMgr.Cleanup(db);
    ASSERT( SQLITE_OK == sqlite3_close(db));
}
