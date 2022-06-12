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

#define UNBOUND 0
#define VARIABLE 1

typedef struct value_t {
    char tag;

    // This sufficies to model variables for the sake of __allium_get_value,
    // but is not a good model of Allium values in general.
    struct value_t *ptr;
} value_t;

// TODO: this should eventually go into a bitcode library and always get
// inlined. For the sake of functional correctness, putting it into the runtime
// library is adequate.
value_t *__allium_get_value(value_t *value) {
    while(value->tag == VARIABLE) {
        value = value->ptr;
    }
    return value;
}
