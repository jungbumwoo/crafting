#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    /*
    OP_CONSTANT: opcode(1 byte), constant 'index'(1 bytes)

    constant values in constants Array.
    */
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,  // this instruction will mean "return from the current func."
} OpCode;

typedef struct {
    /*
    code: pointer to store some other data with instructions

    ref: 14.3.1 Chunk
    */
    int count;
    int capacity;

    uint8_t* code;
    int* lines; // store a separate array of integer that parallels the bytecode.
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif