package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
   private final Stmt.Function declaration;

   // This data sructure is called a "closure" because it "closes over" and hold onto the surrounding variables where the function is declared.
   private final Environment closure;

   private final boolean isInitializer;

   LoxFunction(Stmt.Function declaration, Environment closure, boolean isInitializer) {
      this.isInitializer = isInitializer;
      this.closure = closure;
      this.declaration = declaration;
   }

   LoxFunction bind(LoxInstance instance) {
      Environment environment = new Environment(closure);
      environment.define("this", instance);
      return new LoxFunction(declaration, environment, isInitializer);
   }

   @Override
   public Object call(Interpreter interpreter, List<Object> arguments) {
      Environment environment = new Environment(interpreter.globals);
      for (int i = 0; i < declaration.params.size(); i++) {
         environment.define(declaration.params.get(i).lexeme,
               arguments.get(i));
      }

      try {
         interpreter.executeBlock(declaration.body, environment);
      } catch (Return returnValue) {
         return returnValue.value;
      }

      if (isInitializer) return closure.getAt(0, "this");

      // If it never catches one of these exceptions, it means the function reached the end of its body without hitting a return statement.
      // In that case, it implicitly returns nil.
      return null;
   }

   @Override
   public int arity() {
      return declaration.params.size();
   }

   @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }
}

