* ### `program` -> `declation* EOF` ;
    * Starting Point for the grammar
    * Represents a complete Lox script or REPL entry

* ### `declaration` -> `classDecl` | `funDecl` | `varDecl` | `statement` ;
    * 아래 둘과 같은 케이스로 인해 구분이 필요함 \
    `if (a) print "hello";` -> (O) \
    `if (a) var b = 1;` -> (X)
<br></br>

* ### `classDecl` -> "class" INDENTIFIER "{" function* "}" ;
    * class: leading keyword
    * IDENTIFIER: class name
    * function*: methods
* ### `funDecl` -> `"fun"` function ;
* ### `function` -> `IDENTIFIER` "(" `parameters`? ")" `block` ;
* ### `parameters` -> `IDENTIFIER ( "," IDENTIFIER )*` ;

* ### `statement` -> `exprStmt` | `forStmt` | `ifStmt` | `printStmt`| `returnStmt` | `whileStmt` | `block`;
* ### `returnStmt` -> `"return"` `expression`? `";"` ;
* ### `whileStmt` -> `"while"` `"("` `expression` `")"` `statement` ;
* ### `forStmt` -> `"for"` `"("` ( `varDecl` | `exprStmt` | `";"` ) `expression?` `";"` `expression?` `")"` `statement` ;
* ### `ifStmt` -> `"if"` `"("` `expression` `")"` `statement` ( `"else"` `statement` )? ;
* ### `block` -> `"{"` `declaration*` `"}"` ;
* ### `verDecl` -> `"var" IDENTIFIER ( "=" expression )? ";" ;`
    * The rule for declaring a variable
    * var: leading keyword.
    * IDENTIFIER: variable name. matches a single identifier token.
    * ( "=" expression )?: optional initializer expression.
<br></br>

* ### `primary` -> `"true"` | `"false"` | `"nil"` | NUMBER | STRING | "(" expression ")" ; | IDENTIFIER ;
    * To access a variable, define a new kind of primary expression
    * STRING: string literal
    * "(" expression ")": grouping expression
<br></br>

* ### `exprStmt` -> `expression`;
* ### `printStmt` -> `"print" expression ";" `;

* ### `expression` -> `assignment` ;
* ### `assignment` -> ( `call "."`) ? `IDENTIFIER` `"="` `assignment` | `logic_or` ;
* ### `logic_or` -> `logic_and` ( `"or"` `logic_and` )* ;
* ### `logic_and` -> `equality` ( `"and"` `equality` )*
* ### `equality` -> `comparison` ( ( `"!="` | `"=="` ) `comparison` )`*` ;
* ### `comparison` -> `term` ( ( `">"` | `">="` | `"<"` | `"<="` ) `term` )`*` ;
* ### `term` -> `factor` ( ( `"-"` | `"+"` ) `factor` )`*`;
* ### `factor` -> `unary` ( ( `"/"` | `"*"` ) `unary` )`*`;
* ### `unary` -> ( `"-"` | `"!"` ) `unary` |  `call`;
* ### `call` -> `primary` ( `"("` `arguments?` `")"` )* ;
* ### `arguments` -> `expression ( "," expression )*` ;
* ### `primary` -> `NUMBER` | `STRING` | `"true"` | `"false"` | `"nil"` | `"("` `expression` `")"` | `IDENTIFIER`;

* ### `literal` -> `NUMBER` | `STRING` | `"true"` | `"false"` | `"nil"`;
* ### `grouping` -> `"("` `expression` `")"`;
* ### `binary` -> `expression operator expression `;
* ### `operator` -> `"=="` | `"!="` | `"<"` | `"<="` | `">"` | `">="` | `"+"` | `"-"` | `"*"` | `"/"`;

---
#### Grammer Ref
-   `(a | b) c` -> `ac` or `bc` 
    -   select one from a series of options

- `*` 
    - recursion. allow the previous symbol or group to repeat zero or more times.

- `?`
    - optional production.
---

#### REPL: Read-Eval-Print-Loop
* REPL은 Read-Eval-Print-Loop의 약자로, 사용자가 입력한 코드를 읽고, 평가하고, 출력하고, 다시 반복하는 것을 말한다.

---
#### Ref chapter
8.2.1 \
6.1


---

#### Returning From calls
```
fun count() {
    while (n < 100) {
        if (n == 3) return n;
        print n;
        n = n + 1;
    }
}

count(1);
```

```
<!-- callstack of above func -->
  Interpreter.visitReturnStmt()
  Interpreter.visitIfStmt()
  Interpreter.executeBlock()
  Interpreter.visitBlockStmt()
  Interpreter.visitWhileStmt()
  Interpreter.executeBlock()
  LoxFunction.call()
  Interpreter.visitCallExpr()
```



