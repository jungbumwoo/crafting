8.2.1 

*  ### `program` -> `declation* EOF` ;
    * Starting Point for the grammar
    * Represents a complete Lox script or REPL entry

* ### `declaration` -> `varDecl` | `statement` ;
    * 아래 둘과 같은 케이스로 인해 구분이 필요함 \
    `if (a) print "hello";` -> (O) \
    `if (a) var b = 1;` -> (X)
<br></br>

* ### `statement` -> `exprStmt` | `printStmt` ;

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

* ### `exprStmt` -> `expression ";" `;
* ### `printStmt` -> `"print" expression ";" `;

* ### `expression` -> `literal` | `unary` | `binary` | `grouping` ;
* ### `literal` -> `NUMBER | STRING | "true" | "false" | "nil" `;
* ### `grouping` -> `"(" expression ")" `;
* ### `unary` -> `( "-" | "!" ) expression `;
* ### `binary` -> `expression operator expression `;
* ### `operator` -> `"==" | "!=" | "<" | "<=" | ">" | ">=" | "+" | "-" | "*" | "/" `;

 
---

#### REPL: Read-Eval-Print-Loop
* REPL은 Read-Eval-Print-Loop의 약자로, 사용자가 입력한 코드를 읽고, 평가하고, 출력하고, 다시 반복하는 것을 말한다.