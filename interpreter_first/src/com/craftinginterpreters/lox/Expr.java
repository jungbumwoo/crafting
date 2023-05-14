package com.craftinginterpreters.lox;

abstract class Expr {  // abstract class?
    static class Binary extends Expr {
        final Expr left;
        final Token operator;
        final Expr right;

        Binary (Expr left, Token operator, Expr right) {
            this.left = left;
            this.operator = operator;
            this.right = right;
        }
    }
}
