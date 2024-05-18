#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    va_list args;  // let us pass an arbitrary number of arguments to runtimeError()
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
};

void freeVM() {
};

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
};

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
};

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static InterpreterResult run() {
/*
  * Given a numeric opcode, we need to get to the right C code that implements that instruction's semantics.
 * This process is called dispatching or decoding.
 *
 * for keep it simple, we'll use a switch statement to dispatch to the right code.
 * other options: "direct threading", "jump table", "computed goto"
 * */
#define READ_BYTE() (*vm.ip++) // vm.ip 의 값을 읽고, 그 다음 값을 가리키도록 증가시킴
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(ValueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop());\
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

    printf("== start interpret == \n");
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("              ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                // [opcode], [constant index] 라 switch 문에서 opcode를 읽었고,
                Value constant = READ_CONSTANT(); // 그 다음 바이트를 읽어서 constant value를 가져옴
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;

            case OP_ADD: BINARY_OP(NUMBER_VAL, +); break;
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VALUE(-AS_NUMBER(pop())));
                break;
            }
            case OP_RETURN: {
                // for real func, have to change this.
                // but for now, just print the value.
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;

            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

/*
create a new empty chunk and pass it over to the compiler. 
The compiler will take the user’s program and fill up the chunk with bytecode.

If it does encounter an error, compile() returns false and discard the unusable chunk.
*/
InterpreterResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpreterResult result = run();

    freeChunk(&chunk);
    complie(source);

    return INTERPRET_OK;
}