#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "db.h"

#define DB_FILE_NAME "tick.db"
#define DB_INIT_STR \
    "PRAGMA journal_mode = WAL;" \
    "CREATE TABLE IF NOT EXISTS entries (" \
    "   entry_id   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"\
    "   start_time INTEGER DEFAULT (unixepoch()) NOT NULL," \
    "   stop_time  INTEGER," \
    "   comment    TEXT" \
    ") STRICT;" \
    "CREATE TABLE IF NOT EXISTS tags (" \
    "   tag_id     INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
    "   name       TEXT NOT NULL" \
    ");" \
    "CREATE TABLE IF NOT EXISTS entry_tags (" \
    "   entry_id   INTEGER," \
    "   tag_id     INTEGER," \
    "   FOREIGN KEY (entry_id) REFERENCES entries(entry_id)," \
    "   FOREIGN KEY (tag_id) REFERENCES entries(tag_id)," \
    "   PRIMARY KEY (entry_id, tag_id)" \
    ");"

sqlite3* db_init(void)
{
    sqlite3 *db = NULL;

    const char *home = getenv("HOME");
    if(!home || strlen(home) == 0)
    {
        fprintf(stderr, "ERROR: could not aquire HOME\n");
        goto err;
    }

    char db_dir[1024];
    char db_path[2048];
    snprintf(db_dir, sizeof(db_dir), "%s/.local/share/tick", home);
    mkdir(db_dir, 0700);
    snprintf(db_path, sizeof(db_path), "%s/%s", db_dir, DB_FILE_NAME);

    if(sqlite3_open_v2(
                db_path, 
                &db, 
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 
                NULL
            ) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not open database at path %s\n", DB_FILE_NAME);
        goto err;
    }

    char *errmsg = NULL;
    if(sqlite3_exec(db, DB_INIT_STR, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not initialize database: %s\n", errmsg);
        goto err;
    }

    return db;

err:
    if(db) sqlite3_close(db);
    return NULL;
}
