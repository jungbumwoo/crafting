#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
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
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
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
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;
            case OP_NEGATE: {
                push(-pop());
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

InterpreterResult interpret(Chunk* chunk) {
    vm.chunk = chunk;

    vm.ip = vm.chunk->code;
    return run();
};