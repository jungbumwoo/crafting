package com.craftinginterpreters.lox;

class Token {
    final TokenType type;
    final String lexeme;
    final Object literal;
    final int line;

    Token(TokenType type, String lexeme, Object literal, int line) {
        this.type = type; // TokenType.java
        this.lexeme = lexeme; // The lexemes are only the raw substrings of the source code
        this.literal = literal; // The literal is the actual value of the token for String, Number ...
        this.line = line; // 팔요에 따라 location을 offset, length 로 구현된 곳도 있음
    }

    public String toString() {
        return type + " " + lexeme + " " + literal;
    }
}
