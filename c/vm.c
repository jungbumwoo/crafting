#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
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
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);
};

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
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

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
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

/* 
    yanks the next two bytes from the chunk and builds a 16-bit unsigned integer소
    ip - 메모리 주소
    16비트로 만들기 위해 첫 번째 8비트를 왼쪽으로 8비트 이동시키고, 두 번째 8비트를 더함.
*/
#define READ_SHORT() \
    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))

/*
    reads a one-byte operand from the bytecode chunk.
    It treats that as an index into the chunk's constant table and returns the string at that index.
*/
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
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
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                // takes a single-byte operand for the stack slot where the local lives.
                // It loads the value from that index and then pushes it on top of the stack.

                // GET인데 왜 push 인가? - 값을 가져와서 stack에 push 해서 가져가 쓸 수 있도록 함. 
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]); // OP_SET_LOCAL에서 설정한 값
                break;
            }
            /* 
            It takes the assigned value from the top of the stack and stores in it the stack slot corresponding to the local variable.
            it doesn't pop the value off the stack.
            assignmet is an expression, and every expression produces a value.
            The value of an assignment expression is the assigned value itself, so the VM just leaves the value on the stack.
            */
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) vm.ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                vm.ip -= offset;
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
#undef READ_STRING
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
