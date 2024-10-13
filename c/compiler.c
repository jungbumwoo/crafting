#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign); // simple typedef for a function type that returns nothing.

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth; // zero is global scope, one is the first top-level block ..
} Local;

typedef struct {
    /* 
        In jlox, used a linked chain of "environment" HashMaps to track which local variables were currently in scope.
        For clox, a little closer to the metal.
    */
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

// ParseFn type is a simple typedef for a function type
// that takes no arguments and returns nothing.
typedef void (*ParseFn)();

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true; // Afer an error, go ahead and keep compliling as normal as if the error never occurred.
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();  // scanner doesn’t report lexical errors. Instead, it creates special error tokens and leaves it up to the parser to report them.
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

// It's similar to advance() in that it reads the next token.
// But it also validates that token has an expected type.
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// After we parse and understand a piece of the user’s program, the next step is 
// to translate that to a series of bytecode instructions.
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);  // emits a bytecode instruction and writes a placeholder operand for jump offset.
    emitByte(0xff);  // use two bytes for the jump offset operand. A 16-bit offset jump over up to 65,535 bytes.
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    /*
     * When run clox, it will parse, compile, and execute a single expression, then print the result.
     * To print that value, we are temporarily using the OP_RETURN instruction.
     * */
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t) constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
    // goes back into the bytecode and replaces the operand at the given location.
    // param: offset -> how far to jump.
    // if condition is false, we need to jump over the code.

    // currentChunk()->count: number of bytecode in current chunk
    int jump = currentChunk()->count - offset - 2;  // -2 to adjust for the bytecode for the jump offset itself.

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    // 바이트 코드에 jump 값(16비트)을 두 8비트에 기록
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler() {
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif

}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    // when a block ends, we need to put them to reset.
    while (current -> localCount > 0 &&
              current -> locals[current -> localCount - 1].depth > current -> scopeDepth) {
          emitByte(OP_POP);
          current -> localCount--;
     }
}

// forward declaration
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void parsePrecedence(Precedence precedence) {
    advance();
    /* look up a prefix parser for the current token.
    The first token is always going to belong tot some kind of prefix expression.
    It may turn out to be nested as an operand inside one or more infix expressions,
    but as you read the code from leftt to right, the first token you hit always belongs to a prefix expression.

    ex) -a.b + c;
    */
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    /* 파싱할 때 연산자 우선 순위를 비교.
        할당(=)은 가장 낮은 우선순의를 갖고 항상 마지막에 일어나야함.
        canAssign: 할당이 허용되는지
        가능한 경우: ex) x = a + b;
        불가능한 경우: ex) a + b = c; precedence 값이 높기에 +, *, . 등은 허용되지 않음 
    */
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    /*
    17.6.1 Parsing with precedence
    If the next token is too low precedence, or isn't an infix operator at all, then we're done.
    Otherwise, we parse the infix operator and its right operand.
    It consumes whatever other tokens it needs(usually the right operand) and returns back to parsePrecedence().
    Then we loop back around and see if the next token is also a valid infix operator that can take the entire
    preceding expression as its operand.
    Keep looping like that, crunching through infix operators and their operands until we hit a token that
     isn't an infix operator or is too low precedence and stop.
    */
    while (precedence <= getRule(parser.current.type) -> precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    // If the = doesn't get consumed as part of the expression, nothing else is going to consume it.
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
        expression();
    }
}

// takes the given token and adds its lexeme to the chunk's constant table as a string.
// then, returns the index of that constant in the constant table.
static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a-> length != b-> length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// walk the list of locals that are currently in scope.
// walk the array backward so that we find the last declared variable.
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount -1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                // "Declaring" 후 "Defining" 되지 않은 경우.
                error("Cannot read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

// stores the variable's name and the depth of the scope that owns the variable.
static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    /*
    after we finish compiling the initializer, mark the variable as initialized and ready to use.
    before we finish, set special sentienl value, -1

    ex) var a = "matt";
        (1)
                      (2)

    1) declare uninitialized
    2) ready for use
    */
    // local->depth = current->scopeDepth;
    local->depth = -1;
}

/*
로컬 변수는 함수나 블록 내에서만 유효하므로, 컴파일러는 로컬 변수를 명확하게 추적 반면에, 전역 변수는 프로그램 전체에서 사용됨
전역 변수는 '늦게 바인딩 late bound' - 전역 변수의 값이나 존재 여부가 프로그램 실행 중에 결정될 수 있다.
컴파일 시점에 컴파일러가 전역 변수의 선언을 모두 알고 있을 필요가 없음.
따라서 컴파일러는 전역 변수의 선언을 따로 추적하거나 관리하지 않고, 나중에 프로그램이 실행될 때 전역 변수를 찾음.
*/
static void declareVariable() {
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;

    // error to have two variables with the same name in the same local scope.
    for (int i = current -> localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        // local variable are appended to the array when they're declared,
        // which means the current scope is always at the end of the array.
        // 어차피 end of the array 비교인데 for loop을 사용해야하나?
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static void defineVariable(uint8_t global) {
    /* 
    No code to create a local variable at runtime.
    VM has already executed the code for variable's initializer, 
    and that value is sitting right on top of the stack as the only remaining temporary.
    */
    if (current -> scopeDepth > 0) {
        // "Declaring" is when the variable is added to the scope, and "Defining" is when it becomes available for use.
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) return 0; // At runtime, locals aren't looked up by name.

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    current->locals[current->localCount -1].depth = current->scopeDepth;
}   

static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
}

/*
When a prefix parser function is called, the leading token has already been
consumed.
An infix parser function is even more in medias res—the entire left-hand operand expression has already been compiled 
and the subsequent infix operator consumed.

When run, the VM will execute the left and right operand code, in that order,
leaving their values on the stack. Then it executes the instruction for the
operator. That pops the two values, computes the operation, and pushes the
result.
*/
static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return; // Unreachable.
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    /* 
    IF the user doesn't initialize the variable,
    the compiler implicitly initializes it to nil by emitting an OP_NIL instruction.
    ex) var a;
    */
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

/*
    - condition expression
        (1) OP_JUMP_IF_FALSE -> false 이면 (4) 로 이동
        (2) OP_POP: 조건 결과 stack에서 제거
    - then branch statement
        (3) OP_JUMP -> true 였었을 때 (5)로 이동
        (4) OP_POP: 조건 결과 stack에서 제거
    - else branch statement
        (5) continue..

    위를 다 compile 해둠
*/
static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.)");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // (2)
    statement(); // then branch statement

    int elseJump = emitJump(OP_JUMP); // (3)
    patchJump(thenJump);
    emitByte(OP_POP); // (4)

    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                // Do nothing.
                ;
        }

        advance();
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) sychronize();
}

// statement -> exprStmt | printStmt | block;
static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    /*
     * strtod: 문자 스트링을 double, float 또는 long double 값으로 변환.
     * */
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
    // [0, length -1] 이 아닌 이유는 leading and trailing quotation mark 를 제거하기 위함.
    emitConstant(OBJ_VAL(copyString(
        parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    // uint8_t arg = identifierConstant(&name);
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // compiler가 '=' 이 있으면 setter, 없으면 getter로 구분
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }

    emitBytes(OP_GET_GLOBAL, arg);
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    /*
    The leading - token has been consumed and is sitting in parser.previous.
    We grab the token type from that to note which unary operator we’re dealing with.
    */
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

/*
 * given a token type, let us know how to parse it.
 * let us find the prefix and infix parse functions for that token type.
 * let us find ...
 * the function that will parse the token when it appears at the beginning of an expression.
 *
 * the function to compile a prefix expression starting with a token of that type.
 * the function to compile an infix expression whose left operand is followed by a token of that type
 * the precedence of an infix expression that uses that token as an operator.
 * */
ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
        [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG]          = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, NULL, PREC_NONE},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, NULL, PREC_NONE},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

// Scan -> Parse -> Compile -> Interpret
bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
    return !parser.hadError;
}
