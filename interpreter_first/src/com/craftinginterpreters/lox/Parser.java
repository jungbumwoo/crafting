package com.craftinginterpreters.lox;

import java.util.List;

import static com.craftinginterpreters.lox.TokenType.*;

class Parser {
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;
    private int current = 0;

    Parser (List<Token> tokens) {
        this.tokens = tokens;
    }

    // parses a single expression and returns it.
    // 추후 리펙토링 예정.
    Expr parse() {
        try {
            return expression();
        } catch (ParseError error) {
            // parsing 과정에서 에러가 발생하면 null을 반환한다.
            // 이는 파싱 에러를 무시하고 다음 토큰을 계속해서 파싱하도록 한다.
            // 이렇게 하면 에러가 여러개 발생할 수 있지만, 그래도 최대한 많은 에러를 보고하도록 한다.
            // parser promises not to crash.
            return null;
        }
    }

    private Expr expression() {
        return equality();
    }

    // equality → comparison ( ( "!=" | "==" ) comparison )* ;
    private Expr equality() {
        Expr expr = comparison();

        // left-associative nested tree of binary operator nodes.
        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous(); // the operator
            Expr right = comparison(); // the right operand
            expr = new Expr.Binary(expr, operator, right); // the expression
        }

        return expr;
    }

    // comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    private Expr comparison() {
        Expr expr = term();

        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr term() {
        Expr expr = factor();

        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr factor() {
        Expr expr = unary();

        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();

            return new Expr.Unary(operator, right);
        }

        return primary();
    }

    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE)) return new Expr.Literal(true);
        if (match(NIL)) return new Expr.Literal(null); // nil is null in Java
        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }
        if (match(LEFT_PAREN)) { // grouping
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }
//

    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) { // TokenType 중에서도 어떤 토큰타입인지 구분해야할듯?
                advance(); // consume the token
                return true;
            }
        }

        return false;
    }

    // 마지막 ")" 처리 할 때 쓰임.
    private Token consume(TokenType type, String message) {
        if (check(type)) return advance();

        throw error(peek(), message);
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    // It discards tokens until it finds a statement boundary.
    // After catching a ParseError, synchronize() is called to continue parsing. (parse the rest of the file)
    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) return;

            switch (peek().type) {
                case CLASS:

                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case RETURN:
                    return;
            }

            advance();
        }
    }
}
