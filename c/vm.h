#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

typedef struct {
    Chunk* chunk;

    /*
    ip: instruction pointer at x86, x64, CLR.
    68k, PowerPC, ARM, p-code, JVM call it the program counter (PC).

    ip points to the instruction that is about to be executed.
    always points to the next instruction, not the one currently being handled.
    */
    uint8_t* ip;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpreterResult;

void initVM();
void freeVM();
InterpreterResult interpret(Chunk* chunk);

#endif