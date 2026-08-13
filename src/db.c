#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "db.h"

#define DB_FILE_NAME "tick.db"

#define DB_INIT_STR \
    "PRAGMA journal_mode = WAL;" \
    "PRAGMA foreign_keys = ON;" \
    "CREATE TABLE IF NOT EXISTS entries (" \
    "   entry_id   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
    "   start_time INTEGER NOT NULL," \
    "   stop_time  INTEGER," \
    "   comment    TEXT" \
    ") STRICT;" \
    "CREATE TABLE IF NOT EXISTS tags (" \
    "   tag_id     INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
    "   name       TEXT NOT NULL UNIQUE" \
    ") STRICT;" \
    "CREATE TABLE IF NOT EXISTS entry_tags (" \
    "   entry_id   INTEGER," \
    "   tag_id     INTEGER," \
    "   FOREIGN KEY (entry_id) REFERENCES entries(entry_id) ON DELETE CASCADE," \
    "   FOREIGN KEY (tag_id) REFERENCES tags(tag_id) ON DELETE CASCADE," \
    "   PRIMARY KEY (entry_id, tag_id)" \
    ") STRICT;"

#define DB_INSERT_ENTRY "INSERT INTO entries (start_time, stop_time, comment) VALUES (?, ?, ?);"
// TODO check active tracking
#define DB_START_ENTRY  "INSERT INTO entries (start_time, comment) VALUES (?, ?);"
// TODO check active tracking
#define DB_STOP_ENTRY   "UPDATE entries SET stop_time = ? WHERE entry_id = ?;"
#define DB_DELETE_ENTRY "DELETE FROM entries WHERE entry_id = ?;"

#define DB_INSERT_TAG   "INSERT INTO tags (name) VALUES (?);"
#define DB_DELETE_TAG   "DELETE FROM tags WHERE tag_id = ?;"

static int db_bind_time(sqlite3_stmt *stmt, int pos, time_t time)
{
    if(time >= 0)
    {
        return sqlite3_bind_int64(stmt, pos, time);
    }
    else
    {
        return sqlite3_bind_null(stmt, pos);
    }
}

sqlite3* db_init(void)
{
    sqlite3 *db = NULL;
    char *errmsg = NULL;

    const char *home = getenv("HOME");
    if (!home || strlen(home) == 0)
    {
        fprintf(stderr, "ERROR: could not acquire HOME directory\n");
        goto err;
    }

    char db_dir[1024];
    char db_path[2048];
    snprintf(db_dir, sizeof(db_dir), "%s/.local/share/tick", home);
    mkdir(db_dir, 0700);
    snprintf(db_path, sizeof(db_path), "%s/%s", db_dir, DB_FILE_NAME);

    if (sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not open database at %s: %s\n", db_path, sqlite3_errmsg(db));
        goto err;
    }

    if (sqlite3_exec(db, DB_INIT_STR, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not initialize database schema: %s\n", errmsg);
        goto err;
    }

    return db;

err:
    if(db) sqlite3_close(db);
    if(errmsg) sqlite3_free(errmsg);

    return NULL;
}

int db_entries_add_impl(DB *db, int64_t *entry_id, time_t start_time, time_t stop_time, const char *comment)
{
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare(db, DB_INSERT_ENTRY, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not prepare entry insert: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(db_bind_time(stmt, 1, start_time) != SQLITE_OK ||
        db_bind_time(stmt, 2, stop_time) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 3, comment, -1, SQLITE_STATIC) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not bind parameters: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "ERROR: could not prepare entry insert: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    *entry_id = sqlite3_last_insert_rowid(db);

    sqlite3_finalize(stmt);
    return 0;

err:
    if(stmt) sqlite3_finalize(stmt);
    return 1;
}

int db_entries_add(DB *db, int64_t *entry_id, time_t start_time, time_t stop_time, const char *comment)
{
    return db_entries_add_impl(db, entry_id, start_time, stop_time, comment);
}

int db_entries_start(DB *db, int64_t *entry_id, time_t start_time, const char *comment)
{
    return db_entries_add_impl(db, entry_id, start_time, -1, comment);
}

int db_entries_stop(DB *db, int64_t entry_id, time_t stop_time)
{
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare(db, DB_STOP_ENTRY, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not prepare entry update: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(db_bind_time(stmt, 1, stop_time) != SQLITE_OK ||
        sqlite3_bind_int64(stmt, 2, (sqlite3_int64) entry_id) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not bind parameters: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_step(stmt) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not prepare entry insert: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    sqlite3_finalize(stmt);
    return 0;

err:
    if(stmt) sqlite3_finalize(stmt);
    return 1;
}

int db_entries_delete(DB *db, int64_t entry_id)
{
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare(db, DB_DELETE_ENTRY, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not prepare entry update: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_bind_int64(stmt, 1, (sqlite3_int64) entry_id) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not bind parameters: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "ERROR: could not prepare entry insert: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    sqlite3_finalize(stmt);
    return 0;

err:
    if(stmt) sqlite3_finalize(stmt);
    return 1;
}

int db_tags_add(DB *db, int64_t *tag_id, const char *name)
{
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare(db, DB_INSERT_TAG, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not prepare entry update: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not bind parameters: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "ERROR: could not prepare entry insert: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    *tag_id = sqlite3_last_insert_rowid(db);

    sqlite3_finalize(stmt);
    return 0;

err:
    if(stmt) sqlite3_finalize(stmt);
    return 1;
}

int db_tags_delete(DB *db, int64_t tag_id)
{
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare(db, DB_DELETE_TAG, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not prepare entry update: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_bind_int64(stmt, 1, (sqlite3_int64) tag_id) != SQLITE_OK)
    {
        fprintf(stderr, "ERROR: could not bind parameters: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "ERROR: could not prepare entry insert: %s\n", sqlite3_errmsg(db));
        goto err;
    }

    sqlite3_finalize(stmt);
    return 0;

err:
    if(stmt) sqlite3_finalize(stmt);
    return 1;
}

void db_close(DB *db)
{
    sqlite3_close(db);
}
