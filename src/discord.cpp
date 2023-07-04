#include <sys/stat.h>

#include <iostream>
#include <sstream>

#include <sqlite3.h>

extern "C" {
#include "discord.h"
void discord_error(const char * message);
} // extern "C"


static const char * const cDbPath = "aeaea_discord.sqlite3";

static sqlite3 *sDb;
static sqlite3_stmt *stmt_insert_info;

void discord_info(const char *message)
{
	int rc;
	rc = sqlite3_reset(stmt_insert_info);
	if (SQLITE_OK != rc) {
        discord_error("Couldn't reset statement.");
		return;
	}
	
	rc = sqlite3_clear_bindings(stmt_insert_info);
	if (SQLITE_OK != rc) {
        discord_error("Couldn't clear bindings.");
		return;
	}
	
	int ind = sqlite3_bind_parameter_index(stmt_insert_info, ":message");
	if (ind == 0)
	{
        discord_error("Couldn't bind parameter index.");
		return;
	}
	
	rc = sqlite3_bind_text(stmt_insert_info, ind, message, -1, SQLITE_STATIC);
	if (SQLITE_OK != rc)
	{
		discord_error("Couldn't bind text.");
        return;
	}
	
	rc = sqlite3_step(stmt_insert_info);
	if (SQLITE_OK != rc)
	{
        discord_error("Couldn't step.");
		return;
	}
}


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


static void init_db( sqlite3 *db )
{
	char *zErrMsg = NULL;
	
	std::ostringstream oss;
	oss << "CREATE TABLE info_messages (\n"
		<< "  message TEXT \n"
		<< ");";
	
	std::string str = oss.str();
	
	ASSERTM( SQLITE_OK == sqlite3_exec(db, str.c_str(), 0, 0, &zErrMsg),
            "CREATE TABLE: " << zErrMsg);
}

void discord_init( void )
{
	sqlite3 *db;
	
	bool exists;
	
	{
        struct stat st;
        exists = ::stat(cDbPath, &st) == 0;
	}
	ASSERT( SQLITE_OK == sqlite3_open(cDbPath, &db));
	if (!exists)
	{
		init_db(db);
	}
	
	const char * const str = "INSERT INTO info_messages VALUES (:message);";
	ASSERTM( SQLITE_OK == sqlite3_prepare_v2(db, str, -1, &stmt_insert_info, NULL),
            sqlite3_errmsg(db));
	
	sDb = db;
}
