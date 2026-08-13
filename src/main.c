#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "db.h"

#define shift_args(argc, argv) ((argc)--, *(argv)++)

int perform_start(DB *db, const char *comment)
{
    time_t start = time(NULL);
    
    int64_t entry_id;
    db_entries_start(db, &entry_id, start, comment);

    return 0;    
}

int perform_tag(DB *db, const char *tag)
{
    (void) db;
    (void) tag;

    return 0;    
}

int handle_subcommands(DB *db, int argc, char **argv)
{
    while(argc > 0)
    {
        const char *flag = shift_args(argc, argv);

        if(0 == strcmp(flag, "start"))
        {
            // TODO parse comment
            perform_start(db, NULL);
        }
        else if(0 == strcmp(flag, "stop"))
        {
            assert(0 && "Not implemented");
        }
        else if(0 == strcmp(flag, "edit"))
        {
            assert(0 && "Not implemented");
        }
        else if(0 == strcmp(flag, "delete"))
        {
            assert(0 && "Not implemented");
        }
        else if(0 == strcmp(flag, "tag"))
        {
            assert(0 && "Not implemented");
        }
        else if(0 == strcmp(flag, "sum"))
        {
            assert(0 && "Not implemented");
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *program = shift_args(argc, argv);
    if(0 == argc)
    {
        fprintf(stderr, "USAGE %s <subcommand> <options>\n", program);
        return 1;
    }

    DB *db = db_init();
    assert(db);

    handle_subcommands(db, argc, argv);

    db_close(db);
    
    return 0;
}
