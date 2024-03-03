#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

void initVM() {
};

void freeVM() {
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
    printf("== start interpret == \n");
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                // [opcode], [constant index] 라 switch 문에서 opcode를 읽었고,
                Value constant = READ_CONSTANT(); // 그 다음 바이트를 읽어서 constant index로 사용
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;

            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpreterResult interpret(Chunk* chunk) {
    vm.chunk = chunk;

    vm.ip = vm.chunk->code;
    return run();
};