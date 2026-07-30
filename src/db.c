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
#define DB_START_ENTRY  "INSERT INTO entries (start_time, comment) VALUES (?, ?);"
#define DB_STOP_ENTRY   "UPDATE entries SET stop_time = ? WHERE entry_id = ?;"
#define DB_DELETE_ENTRY "DELETE FROM entries WHERE entry_id = ?;"

#define DB_INSERT_TAG   "INSERT INTO tags (name) VALUES (?);"
#define DB_DELETE_TAG   "DELETE FROM tags WHERE tag_id = ?;"

struct entry_add_args  
{ 
    time_t start; 
    time_t stop; 
    const char *comment; 
};

struct entry_start_args 
{ 
    time_t start; 
    const char *comment; 
};

struct entry_stop_args  
{ 
    int64_t id; 
    time_t stop; 
};

static int mkdir_p(const char *path, mode_t mode)
{
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/') tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) 
    {
        if (*p == '/') 
        {
            *p = 0;
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;

    return 0;
}

static int db_exec_stmt(sqlite3 *db, const char *sql, void (*bind_fn)(sqlite3_stmt*, void*), void *arg)
{
    sqlite3_stmt *stmt = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) 
    {
        fprintf(stderr, "ERROR: prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    if (bind_fn) bind_fn(stmt, arg);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) 
    {
        fprintf(stderr, "ERROR: execution failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    return 0;
}

static void bind_entry_add(sqlite3_stmt *stmt, void *arg) 
{
    struct entry_add_args *a = arg;
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)a->start);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)a->stop);
    sqlite3_bind_text(stmt, 3, a->comment, -1, SQLITE_STATIC);
}

static void bind_entry_start(sqlite3_stmt *stmt, void *arg) 
{
    struct entry_start_args *a = arg;
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)a->start);
    sqlite3_bind_text(stmt, 2, a->comment, -1, SQLITE_STATIC);
}

static void bind_entry_stop(sqlite3_stmt *stmt, void *arg) 
{
    struct entry_stop_args *a = arg;
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)a->stop);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)a->id);
}

static void bind_single_id(sqlite3_stmt *stmt, void *arg) 
{
    sqlite3_bind_int64(stmt, 1, *(int64_t*)arg);
}

static void bind_single_text(sqlite3_stmt *stmt, void *arg) 
{
    sqlite3_bind_text(stmt, 1, (const char*)arg, -1, SQLITE_STATIC);
}

sqlite3* db_init(void)
{
    sqlite3 *db = NULL;
    char *errmsg = NULL;

    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') 
    {
        fprintf(stderr, "ERROR: could not acquire HOME directory\n");
        return NULL;
    }

    char db_dir[1024];
    char db_path[2048];
    snprintf(db_dir, sizeof(db_dir), "%s/.local/share/tick", home);
    
    if (mkdir_p(db_dir, 0700) != 0) 
    {
        fprintf(stderr, "ERROR: failed to create directory %s\n", db_dir);
        return NULL;
    }

    snprintf(db_path, sizeof(db_path), "%s/%s", db_dir, DB_FILE_NAME);

    if (sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) 
    {
        fprintf(stderr, "ERROR: could not open database at %s: %s\n", db_path, sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return NULL;
    }

    if (sqlite3_exec(db, DB_INIT_STR, NULL, NULL, &errmsg) != SQLITE_OK) 
    {
        fprintf(stderr, "ERROR: could not initialize database schema: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return NULL;
    }

    return db;
}

int db_entries_add(sqlite3 *db, time_t start_time, time_t stop_time, const char *comment, int64_t *entry_id)
{
    struct entry_add_args args = { start_time, stop_time, comment };
    if (db_exec_stmt(db, DB_INSERT_ENTRY, bind_entry_add, &args) != 0) return 1;

    if (entry_id) *entry_id = sqlite3_last_insert_rowid(db);

    return 0;
}

int db_entries_start(sqlite3 *db, time_t start_time, const char *comment, int64_t *entry_id)
{
    struct entry_start_args args = { start_time, comment };
    if (db_exec_stmt(db, DB_START_ENTRY, bind_entry_start, &args) != 0) return 1;

    if (entry_id) *entry_id = sqlite3_last_insert_rowid(db);

    return 0;
}

int db_entries_stop(sqlite3 *db, int64_t entry_id, time_t stop_time)
{
    struct entry_stop_args args = { entry_id, stop_time };
    return db_exec_stmt(db, DB_STOP_ENTRY, bind_entry_stop, &args);
}

int db_entries_delete(sqlite3 *db, int64_t entry_id)
{
    return db_exec_stmt(db, DB_DELETE_ENTRY, bind_single_id, &entry_id);
}

int db_tags_add(sqlite3 *db, const char *name, int64_t *tag_id)
{
    if (db_exec_stmt(db, DB_INSERT_TAG, bind_single_text, (void*)name) != 0) return 1;

    if (tag_id) *tag_id = sqlite3_last_insert_rowid(db);

    return 0;
}

int db_tags_delete(sqlite3 *db, int64_t tag_id)
{
    return db_exec_stmt(db, DB_DELETE_TAG, bind_single_id, &tag_id);
}
