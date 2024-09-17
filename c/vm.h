#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;

    /*
    ip: instruction pointer at x86, x64, CLR.
    68k, PowerPC, ARM, p-code, JVM call it the program counter (PC).

    ip points to the instruction that is about to be executed.
    always points to the next instruction, not the one currently being handled.
    */
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Table strings;

    Obj* objects; // pointer to the head of the list
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpreterResult;

extern VM vm;  // expose external

void initVM();
void freeVM();
InterpreterResult interpret(const char* source);
void push(Value value);
Value pop();

#endif