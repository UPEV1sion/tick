#ifndef DB_H_
#define DB_H_

#include <stdint.h>
#include <time.h>

#include "sqlite3.h"

sqlite3* db_init();

int db_entries_add(sqlite3 *db, time_t start_time, time_t stop_time, const char *comment, int64_t *entry_id);
int db_entries_start(sqlite3 *db, time_t start_time, const char *comment, int64_t *entry_id);
int db_entries_stop(sqlite3 *db, int64_t entry_id, time_t stop_time);
int db_entries_delete(sqlite3 *db, int64_t entry_id);

int db_tags_add(sqlite3 *db, const char *name, int64_t *tag_id);
int db_tags_delete(sqlite3 *db, int64_t tag_id);

#endif // DB_H_
