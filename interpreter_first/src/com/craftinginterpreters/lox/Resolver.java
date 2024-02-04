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
    private FunctionType currentFunction = FunctionType.NONE;

    Resolver(Interpreter interpreter){
        this.interpreter = interpreter;
    }

    private enum FunctionType{
        NONE,
        FUNCTION,
        INITIALIZER,
        METHOD,
    }

    private enum ClassType{
        NONE,
        CLASS,
    }

    private ClassType currentClass = ClassType.NONE;

    @Override
    public Void visitBlockStmt(Stmt.Block stmt){
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        ClassType enclosingClass = currentClass;
        currentClass = ClassType.CLASS;

        declare(stmt.name);
        define(stmt.name);

        if (stmt.superclass != null && stmt.name.lexeme.equals(stmt.superclass.name.lexeme)){
            Lox.error(stmt.superclass.name, "A class cannot inherit from itself.");
        }

        if (stmt.superclass != null) {
            resolve(stmt.superclass);
        }

        beginScope();
        scopes.peek().put("this", true);

        for (Stmt.Function method : stmt.methods){
            FunctionType declaration = FunctionType.METHOD;
            if (method.name.lexeme.equals("init")){
                declaration = FunctionType.INITIALIZER;
            }
            resolveFunction(method, declaration);
        }

        endScope();
        currentClass = enclosingClass;
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
        resolveFunction(stmt, FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt){
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt){
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) resolve(stmt.elseBranch);
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt){
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt){
        if (currentFunction == FunctionType.NONE){
            Lox.error(stmt.keyword, "Can't return from top-level code.");
        }
        if (stmt.value != null) {
            if (currentFunction == FunctionType.INITIALIZER){
                Lox.error(stmt.keyword, "Can't return a value from an initializer.");
            }
            resolve(stmt.value);
        }
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt){
        resolve(stmt.condition);
        resolve(stmt.body);
        return null;
    }

    @Override
    public Void visitBinaryExpr(Expr.Binary expr){
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr){
        resolve(expr.callee);

        for (Expr argument : expr.arguments){
            resolve(argument);
        }

        return null;
    }

    @Override
    public Void visitGetExpr(Expr.Get expr){
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitGroupingExpr(Expr.Grouping expr){
        resolve(expr.expression);
        return null;
    }

    @Override
    public Void visitLiteralExpr(Expr.Literal expr){
        return null;
    }

    @Override
    public Void visitLogicalExpr(Expr.Logical expr){
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitSetExpr(Expr.Set expr){
        resolve(expr.value);
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitThisExpr(Expr.This expr) {
        if (currentClass == ClassType.NONE){
            Lox.error(expr.keyword, "Can't use 'this' outside of a class.");
            return null;
        }
        resolveLocal(expr, expr.keyword);
        return null;
    }

    @Override
    public Void visitUnaryExpr(Expr.Unary expr){
        resolve(expr.right);
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
        if (scope.containsKey(name.lexeme)){
            Lox.error(name, "Already a variable with this name in this scope.");
        }
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

    private void resolveFunction(Stmt.Function function, FunctionType type){
        FunctionType enclosingFunction = currentFunction;
        currentFunction = type;
        beginScope();
        for (Token param : function.params){
            declare(param);
            define(param);
        }
        resolve(function.body);
        endScope();
        currentFunction = enclosingFunction;
    }
}
