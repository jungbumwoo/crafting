package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

// Environment is data structure that stores variables and their values.
public class Environment {
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    // Constructor for global scope.
    Environment() {
        enclosing = null;
    }

    // Constructor creates a new local scope nested inside the given outer one.
    Environment(Environment enclosing){
        this.enclosing = enclosing;  // outer scope
    }

    Object get(Token name){
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        if (enclosing != null) return enclosing.get(name);

        // Syntax Error? Runtime Error? 이건 구현에 따라 선택지임.
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    // 변수 명(name) 에 변수 값(value) 설정
    // get() 과 다른 점은 이미 존재하는 변수에 대해서만 값을 설정한다. 새로 변수를 생성하지 않음.
    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    // 변수 명(name) 에 변수 값(value) 설정
    void define(String name, Object value){
        values.put(name, value);
    }

    Environment ancestor(int distance){
        Environment environment = this;
        for (int i = 0; i < distance; i++){
            environment = environment.enclosing;
        }
        return environment;
    }

    Object getAt(int distance, String name){
        return ancestor(distance).values.get(name);
    }
}
