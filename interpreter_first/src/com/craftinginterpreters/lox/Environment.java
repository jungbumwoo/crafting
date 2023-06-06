package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

// Environment is data structure that stores variables and their values.
public class Environment {
    private final Map<String, Object> values = new HashMap<>();

    Object get(Token name){
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }
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

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    // 변수 명(name) 에 변수 값(value) 설정
    void define(String name, Object value){
        values.put(name, value);
    }
}
