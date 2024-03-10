#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

//
/*
 * Scanner chews through the users source code, it tracks how far it's gone.
 * start: pointer marks the beginning of the current lexeme being scanned.
 * current: pointer marks the character currently being looked at.
 * */
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;


Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}