#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "db.h"

#define shift_args(argc, argv) ((argc)--, *(argv)++)

int perform_start(sqlite3 *db)
{
    (void) db;


    return 0;    
}

int handle_subcommands(sqlite3 *db, int argc, char **argv)
{
    while(argc > 0)
    {
        const char *flag = shift_args(argc, argv);

        if(0 == strcmp(flag, "start"))
        {
            perform_start(db);
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

    sqlite3 *db = db_init();
    assert(db);

    handle_subcommands(db, argc, argv);

    sqlite3_close(db);
    
    return 0;
}
