# Plang
A Toy programming language named Plang (short for Programming lang) written in C for the purpose of understanding the concepts of language design and implementation. 

## Grammar rules
The blocks below define the grammar for Plang.
Terminals are defined between quotes (i.e., "var"). Nonterminals are defined as words starting with an uppercase character.
All capitalized words can be seen as macros. Here, they are left unexpanded to preserve readability. 
Furthermore, a variant of the Baukus-Naur form (BNF) is used for the grammar, which defines the following constructs:
- (*word*)* means 0 or more times *word*
- (*word*)+ means 1 or more times *word*
- (*word*)? means 0 or 1 time *word*
- (*A* | *B*) means choose either *A* or *B*

### Toplevel statements
```ABNF
Program     = (Declaration ";")*
Declaration = VarDecl | 
              Stmt
VarDecl     = "var" Identifier ("=" Expression)?
Stmt        = PrintStmt | 
              Expression
PrintStmt   = "print" Expression
```

### Expressions
```ABNF
Expression  = Equality |
              Assignment |
              Ternary
Assignment  = Identifier "=" Expression
Ternary     = Expression "?" Expression : Expression

Equality    = Comparison (("!=" | "==") Comparison)*
Comparison  = Term (("<" | "<=" | "=>" | ">") Term)*
Term        = Factor (("+" | "-") Factor)*
Factor      = Unary (("*" | "/") Unary)*
Unary       = (("!" | "-") Unary)* |
              Primary
Primary     = Number |
              String |
              Boolean |
              "(" Expression ")" |
              Nil |
              Identifier
```

### Primary types 
```ABNF
Number      = DIGIT+ ("." DIGIT+)?
String      = "\"" (CHAR)* "\""
Boolean     = "true" | "false"
Nil         = "nil" |
Identifier  = ALPHA (ALPHA | DIGIT)*
```

