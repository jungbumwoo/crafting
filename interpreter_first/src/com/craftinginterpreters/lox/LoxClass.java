package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements LoxCallable {
    final String name;

    LoxClass(String name){
        this.name = name;
    }

    @Override
    public String toString(){
        return "class: " + name;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments){
        LoxInstance instance = new LoxInstance(this);
        return instance;
    }

    @Override
    public int arity(){ // arity() mathod is how interpreter validates that you passed the right number of arguments to callable.
        return 0;
    }
}
