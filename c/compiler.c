#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

// Scan -> Parse -> Compile -> Interpret
void compile(const char* source) {
    initScanner(source);
}