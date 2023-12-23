package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

/*
* A block statement introduces a new scope for the statements it contains.
* A function declaration introduces a new scope for its body and binds its parameters in that scope.
* A variable declaration adds a new variable to the current scope.
* Variable and assignment expressioins need to have their variables resolved.
*
* */
class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void>{
    private final Interpreter interpreter;
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();

    Resolver(Interpreter interpreter){
        this.interpreter = interpreter;
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt){
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt){
        declare(stmt.name);
        if (stmt.initializer != null){
            resolve(stmt.initializer);
        }
        define(stmt.name); // set the variable's value to true in the scope map to `true` to mark it as fully initialized.
        return null;
    }

    @Override
    public Void visitVariableExpr(Expr.Variable expr){
        if (!scopes.isEmpty() && scopes.peek().get(expr.name.lexeme) == Boolean.FALSE){  // check to see if the variable is being accessed inside its own initializer.
            Lox.error(expr.name, "Can't read local variable in its own initializer."); // means we have declared it but not yet defined it.
        }

        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitAssignExpr(Expr.Assign expr){
        resolve(expr.value);  // resolve the expression for the assigned value in case it also contains references to the variable.
        resolveLocal(expr, expr.name);  // resolve the variable being assigned to.
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt){
        // decalre and define the name of the function in the current scope.
        declare(stmt.name);
        define(stmt.name);

        // Unlike variables, we define the name eagerly, before resolving the function's body.
        // This lets a function rescursively refer to itself inside its own body.
        resolveFunction(stmt);
        return null;
    }

    void resolve(List<Stmt> statements){
        for (Stmt statement : statements){
            resolve(statement);
        }
    }

    private void resolve(Stmt stmt){
        stmt.accept(this);
    }

    private void resolve(Expr expr){
        expr.accept(this);
    }

    private void beginScope(){
        scopes.push(new HashMap<String, Boolean>());
    }

    private void endScope(){
        scopes.pop();
    }

    /*
    * Declaration adds the variable, we resolve to the innermost scope so that
    * it shadows any outer one and so that we know the variable exists.
    */
    private void declare(Token name){
        if (scopes.isEmpty()) return;

        Map<String, Boolean> scope = scopes.peek();
        scope.put(name.lexeme, false); // Mark is as "not ready yet" by binding its name to false in the scope map.
    }

    private void define(Token name) {
        if (scopes.isEmpty()) return;
        scopes.peek().put(name.lexeme, true);
    }

    private void resolveLocal(Expr expr, Token name){
        for (int i = scopes.size() - 1; i >= 0; i--){
            if (scopes.get(i).containsKey(name.lexeme)){
                interpreter.resolve(expr, scopes.size() - 1 - i); // interpreter.resolve() will implement later.
                return;
            }
        }

        // Not found. Assume it is global.
    }

    private void resolveFunction(Stmt.Function function){
        beginScope();
        for (Token param : function.params){
            declare(param);
            define(param);
        }
        resolve(function.body);
        endScope();
    }
}
