# TSPP Language Grammar Specification

## 1. Program Structure
```ebnf
Program         → DeclarationList? EOF
DeclarationList → (Declaration | NamespaceDecl)*

Declaration     → AttributeList? (
                  VarDecl 
                | FuncDecl 
                | ClassDecl 
                | EnumDecl 
                | TypedefDecl 
                | Statement
                )

AttributeList   → (AttributePrefix Attribute)*
AttributePrefix → "#"
Attribute       → SimpleName
                | SimpleName "(" AttributeArg ")"
AttributeArg    → NUMBER | STRING | IDENTIFIER
```

## 2. Memory Management and Types
```ebnf
VarDecl        → StorageClass? ("let" | "const") IDENTIFIER TypeAnnotation? 
                ("=" Initializer)? ";"

StorageClass   → "#stack" | "#heap" | "#static"

TypeAnnotation → ":" Type

Type           → PrimaryType
                | UnionType
                | ArrayType
                | PointerType
                | ReferenceType
                | TemplateType
                | FunctionType
                | SmartPointerType

PrimaryType    → "void" | "int" | "float" | "bool" | "string" 
                | QualifiedName

UnionType      → Type "|" Type
ArrayType      → Type "[" (Expression)? "]"
PointerType    → Type "@" PointerModifier?
PointerModifier→ "unsafe" | "aligned" "(" NUMBER ")"
ReferenceType  → Type "&"
SmartPointerType → "#shared" "<" Type ">"
                 | "#unique" "<" Type ">"
                 | "#weak" "<" Type ">"

QualifiedName  → IDENTIFIER ("." IDENTIFIER)*
```

## 3. Functions and Parameters
```ebnf
FuncDecl       → FuncModifier* "function" IDENTIFIER 
                GenericParams? "(" ParameterList? ")" 
                TypeAnnotation
                ("throws" TypeList)?
                ("where" ConstraintList)?
                Block

FuncModifier   → "#inline" | "#virtual" | "#unsafe" | "#simd"
                | "#const" | "#target" "(" STRING ")"

GenericParams  → "<" GenericParamList ">"
GenericParamList→ GenericParam ("," GenericParam)*
GenericParam   → IDENTIFIER ("extends" TypeConstraint)?

ParameterList  → Parameter ("," Parameter)*
Parameter      → ParamModifier? IDENTIFIER TypeAnnotation ("=" Expression)?
ParamModifier  → "ref" | "const"

TypeConstraint → "number" | "comparable" | Type
ConstraintList → Constraint ("," Constraint)*
Constraint     → IDENTIFIER ":" TypeConstraint
```

## 4. Classes and Interfaces
```ebnf
ClassDecl      → ClassModifier* "class" IDENTIFIER 
                GenericParams? 
                ("extends" TypeList)? 
                ("implements" TypeList)?
                "{" ClassMember* "}"

ClassModifier  → "#aligned" "(" NUMBER ")"
                | "#packed"
                | "#abstract"

ClassMember    → AccessModifier? MemberDecl
AccessModifier → "public" | "private" | "protected"
MemberDecl     → ConstructorDecl
                | MethodDecl
                | PropertyDecl
                | FieldDecl

FieldDecl      → AttributeList? VarDecl

MethodDecl     → AttributeList? FuncDecl

PropertyDecl   → "get" IDENTIFIER TypeAnnotation Block
                | "set" IDENTIFIER "(" Parameter ")" Block

InterfaceDecl  → "#zerocast"? "interface" IDENTIFIER 
                GenericParams? 
                ("extends" TypeList)? 
                "{" InterfaceMember* "}"

InterfaceMember→ MethodSignature | PropertySignature
```

## 5. Control Flow and Statements
```ebnf
Statement      → ExprStmt
                | Block
                | IfStmt
                | SwitchStmt
                | LoopStmt
                | TryStmt
                | JumpStmt
                | AssemblyStmt

Block          → "{" Statement* "}"

IfStmt         → "if" "(" Expression ")" Statement 
                ("else" Statement)?

SwitchStmt     → "switch" "(" Expression ")" 
                "{" SwitchCase* "}"
SwitchCase     → ("case" Expression | "default") ":" Statement*

LoopStmt       → WhileStmt
                | DoWhileStmt
                | ForStmt
                | ForEachStmt

WhileStmt      → "while" "(" Expression ")" Statement
DoWhileStmt    → "do" Statement "while" "(" Expression ")" ";"
ForStmt        → "for" "(" ForInit? ";" Expression? ";" 
                Expression? ")" Statement
ForEachStmt    → "for" "(" ("let"|"const") IDENTIFIER "of" 
                Expression ")" Statement

TryStmt        → "try" Block CatchClause* FinallyClause?
CatchClause    → "catch" "(" Parameter ")" Block
FinallyClause  → "finally" Block

JumpStmt       → "return" Expression? ";"
                | "break" IDENTIFIER? ";"
                | "continue" IDENTIFIER? ";"
                | "throw" Expression ";"

AssemblyStmt   → "#asm" "(" STRING ("," AsmConstraint)* ")" ";"
```

## 6. Expressions
```ebnf
Expression     → AssignmentExpr

AssignmentExpr → ConditionalExpr
                | UnaryExpr AssignmentOperator AssignmentExpr

ConditionalExpr→ LogicalOrExpr 
                ("?" Expression ":" ConditionalExpr)?

LogicalOrExpr  → LogicalAndExpr ("||" LogicalAndExpr)*
LogicalAndExpr → BitwiseOrExpr ("&&" BitwiseOrExpr)*
BitwiseOrExpr  → BitwiseXorExpr ("|" BitwiseXorExpr)*
BitwiseXorExpr → BitwiseAndExpr ("^" BitwiseAndExpr)*
BitwiseAndExpr → EqualityExpr ("&" EqualityExpr)*
EqualityExpr   → RelationalExpr (("==" | "!=") RelationalExpr)*
RelationalExpr → ShiftExpr (("<" | ">" | "<=" | ">=") ShiftExpr)*
ShiftExpr      → AdditiveExpr (("<<" | ">>") AdditiveExpr)*
AdditiveExpr   → MultiplicativeExpr (("+"|"-") MultiplicativeExpr)*
MultiplicativeExpr → UnaryExpr (("*"|"/"|"%") UnaryExpr)*

UnaryExpr      → PostfixExpr
                | ("++" | "--") UnaryExpr
                | ("+" | "-" | "!" | "~") UnaryExpr
                | PointerExpr

PointerExpr    → "@" UnaryExpr              // Address-of
                | "*" UnaryExpr              // Dereference
                | "cast" "<" Type ">" UnaryExpr

PostfixExpr    → PrimaryExpr
                | PostfixExpr "++"
                | PostfixExpr "--"
                | PostfixExpr "[" Expression "]"
                | PostfixExpr "(" Arguments? ")"
                | PostfixExpr "." IDENTIFIER
                | PostfixExpr "@" IDENTIFIER
                | PostfixExpr "<" TypeList ">"

PrimaryExpr    → IDENTIFIER
                | Literal
                | "(" Expression ")"
                | ArrayLiteral
                | "new" Type Arguments?
                | "this"
                | "null"
                | "undefined"
                | CompileTimeExpr

CompileTimeExpr→ "#const" Expression
                | "#sizeof" "<" Type ">"
                | "#alignof" "<" Type ">"
                | "#typeof" "(" Expression ")"

Literal        → NUMBER 
                | STRING 
                | "true" 
                | "false"
                | CharacterLiteral

ArrayLiteral   → "[" (Expression ("," Expression)*)? "]"
Arguments      → Expression ("," Expression)*
```

## 7. Special Operators and Symbols
```ebnf
AssignmentOperator → "=" | "+=" | "-=" | "*=" | "/=" | "%="
                    | "<<=" | ">>=" | "&=" | "^=" | "|="

TypeList       → Type ("," Type)*

IDENTIFIER     → [a-zA-Z_][a-zA-Z0-9_]*
NUMBER         → [0-9]+ ("." [0-9]+)? ([eE][+-]?[0-9]+)?
                | "0x" [0-9a-fA-F]+
                | "0b" [01]+
STRING         → "\"" [^"]* "\""
CharacterLiteral → "'" . "'"
```

Key changes made:
1. Simplified primary types to just `void`, `int`, `float`, `boolean`, and `string`
2. Removed explicit sized integer and floating-point types
3. Maintained the attribute system to allow compiler optimizations
4. Kept all performance-related attributes and features
5. Let the compiler handle type sizes based on usage context

The compiler can now:
- Choose optimal sizes for integers and floats based on usage
- Apply platform-specific optimizations
- Utilize SIMD operations automatically
- Optimize memory layout without explicit size specifications

Would you like me to show examples of how the compiler would optimize these basic types in different contexts?

conditional, Bitwise expr, cast, compileTimeExpr, undefined and null decls