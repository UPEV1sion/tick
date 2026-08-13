#ifndef DB_H_
#define DB_H_

#include <stdint.h>
#include <time.h>

#include "sqlite3.h"

typedef sqlite3 DB;

DB* db_init();

int db_entries_add(DB *db, int64_t *entry_id, time_t start_time, time_t stop_time, const char *comment);
int db_entries_start(DB *db, int64_t *entry_id, time_t start_time, const char *comment);
int db_entries_stop(DB *db, int64_t entry_id, time_t stop_time);
int db_entries_delete(DB *db, int64_t entry_id);

int db_tags_add(DB *db, int64_t *tag_id, const char *name);
int db_tags_delete(DB *db, int64_t tag_id);

void db_close(DB *db);

#endif // DB_H_
