package com.craftinginterpreters.lox;

class Token {
    final TokenType type;
    final String lexeme;
    final Object literal;
    final int line;

    Token(TokenType type, String lexeme, Object literal, int line) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line; // 팔요에 따라 location을 offset, length 로 구현된 곳도 있음
    }

    public String toString() {
        return type + " " + lexeme + " " + literal;
    }
}
