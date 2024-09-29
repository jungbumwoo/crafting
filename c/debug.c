#include <stdio.h>
#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== debug(disassembleChunk): %s ==\n", name);
    
    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
    printf("==== disassembleChunk done. ====\n");
    printf("\n");
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/*

%-16s %4d

%s: 문자열을 출력하는 서식 지정자입니다. %s 뒤에 -가 붙어 있으므로, 문자열을 왼쪽 정렬하여 출력합니다.
%-16s: 문자열을 출력할 때 총 16자리를 할당하고, 왼쪽으로 정렬합니다. 문자열의 길이가 16보다 작을 경우 공백으로 채웁니다.
%d: 정수를 출력하는 서식 지정자입니다. %d 뒤에 4가 붙어 있으므로, 총 4자리를 할당하고, 오른쪽으로 정렬합니다. 숫자의 길이가 4보다 작을 경우 앞에 공백으로 채웁니다.

따라서 이 서식 지정자들을 함께 사용하면 name 변수에 저장된 문자열을 왼쪽 정렬하여 최대 16자리로 출력하고, 그 뒤에 정수값인 constant를 오른쪽 정렬하여 최대 4자리로 출력합니다.
*/
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    /*
    @param name: The name of the instruction. ex) OP_CONSTANT
    @param chunk: The chunk to disassemble
    @param offset: The offset of the instruction

    (ref: 14.5.3 Constat instructions)
    chunk-> code: [(0) , ... ,(offset) constant instruction, (offset+1) const value, (n) , (n+1), ...]
    */

    // constant보다 constant index가 좀 더 좋은 네이밍으로 보임
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2; // go to next instruction
}

int disassembleInstruction(Chunk* chunk, int offset) {
    /*
    %d: 정수를 출력하는 서식 지정자입니다.
    0: 필드를 채우는 데 사용할 패딩 문자입니다. 여기서는 0으로 설정되어 있으므로, 출력될 숫자의 왼쪽에 0이 추가됩니다.
    4: 출력될 숫자의 최소 길이를 나타냅니다. 숫자가 지정된 최소 길이보다 짧을 경우, 왼쪽에 패딩 문자로 지정된 값으로 채워집니다.

    출력될 숫자의 최소 길이는 4자리입니다.
    숫자가 4자리보다 짧을 경우 왼쪽에 0으로 채웁니다.
    */
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        // for any instruction that shares the same line as the previous one, we just print a vertical bar.
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
    
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}