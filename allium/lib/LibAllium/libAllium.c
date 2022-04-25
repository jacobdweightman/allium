#include <stdio.h>
#include <stdlib.h>

int logLevel;

// Programs which contain a `main` predicate will call this function before any
// other code is run.
void allium_init() {
    const char *s = getenv("ALLIUM_LOG_LEVEL");
    if(s) {
        logLevel = atoi(s);
    }
}
