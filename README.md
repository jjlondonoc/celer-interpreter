# Celer — C-like Interpreter in C

**Celer** is a lightweight, statically-typed, C-inspired interpreted language implemented entirely in **C**.
It was created to explore how a real language can be built from scratch — from lexer and parser to AST and evaluator — while maintaining a simple, efficient, and educational design.

---

## Philosophy

> *“C-like in spirit: control, efficiency, and execution close to the metal.”*

Celer offers:

* Static typing with a small set of primitive types.
* Familiar syntax for anyone who knows C.
* A clear and modular architecture:  `token → lexer → parser → AST → evaluator → REPL`.
* Fully written in portable, dependency-free C99.

---

## Language Specification

### Data Types

| Type     | Description             | Example(s)             |
| -------- | ----------------------- | ---------------------- |
| `int`    | Integer                 | `0`, `-12`, `42`       |
| `float`  | Floating point          | `3.14`, `-0.5`, `10.0` |
| `bool`   | Boolean                 | `true`, `false`        |
| `string` | Text enclosed in quotes | `"hello"`, `"hola\n"`  |

---

### Literals and Escapes

Strings support escape sequences:

| Escape | Meaning      |
| :----- | :----------- |
| `\n`   | newline      |
| `\t`   | tab          |
| `\\`   | backslash    |
| `\"`   | double quote |

Example:

```celer
const msg : string = "línea1\nlínea2\tTAB\\slash\"quote";
```

---

### Comments

```celer
*-- This is a line comment

/* This is a
   block comment */
```

---

### Variables and Constants

Declarations are always terminated with `;`.

```celer
variable x : int = 10;
const greeting : string = "hola";
```

`variable` defines a mutable value;
`const` defines an immutable constant.

---

### Functions

Defined using the keyword **Function** (with capital F).

```celer
Function suma(a : int, b : int) -> int {
  return a + b;
}
```

* Parameters use the same `name : type` format.
* The return type follows `->`.
* Functions are first-class and can be declared in any order.

---

### Control Flow

#### **if / else**

```celer
if (x > 0) {
  print("Positive");
} else {
  print("Non-positive");
}
```

#### **for loops**

Celer supports two `for` forms:

**While-like:**

```celer
for (x < 10) {
  print(x);
  x = x + 1;
}
```

**C-like:**

```celer
for (variable i : int = 0; i < 3; i = i + 1) {
  print("i =", i);
}
```

Supports `break` and `continue`.

---

### Ternary Operator

Special structured ternary:

```celer
condition ? { true: expr_if_true : false: expr_if_false };
```

Example:

```celer
variable y : int = x > 5 ? { true: 1 : false: 0 };
```

---

### Operators

| Category   | Operators                                  |   |        |
| :--------- | :----------------------------------------- | - | ------ |
| Arithmetic | `+`, `-`, `*`, `/`, `%`                    |   |        |
| Comparison | `==`, `!=`, `<`, `<=`, `>`, `>=`           |   |        |
| Logical    | `&&`, `                                    |   | `, `!` |
| Assignment | `=`, `+=`, `-=`, `*=`, `/=`, `%=`          |   |        |
| Others     | `()`, `{}`, `[]`, `,`, `;`, `:`, `?`, `->` |   |        |

---

### Built-in Functions

| Function     | Description                                                      |
| ------------ | ---------------------------------------------------------------- |
| `print(...)` | Prints all arguments separated by spaces, followed by a newline. |

Example:

```celer
print("hello", 42, true);
```

Output:

```
hello 42 true
```

---

## Project Structure

```
celer/
├── include/
│   ├── token.h      # Token type definitions
│   ├── lexer.h      # Lexical analyzer
│   ├── ast.h        # Abstract Syntax Tree structures
│   ├── parser.h     # Recursive-descent parser
│   ├── value.h      # Runtime value representation
│   ├── env.h        # Variable/function environment
│   ├── eval.h       # AST evaluator (interpreter)
│
├── src/
│   ├── token.c
│   ├── lexer.c
│   ├── ast.c
│   ├── parser.c
│   ├── value.c
│   ├── env.c
│   ├── eval.c
│   ├── run.c        # Runs .celer files (main runner)
│   └── repl.c       # Interactive REPL
│
├── examples/
│   ├── demo.celer   # Comprehensive example
│   └── mini.celer   # Minimal example
│
├── build/           # Compiled executables (ignored in Git)
└── README.md
```

---

## File Responsibilities

| File                    | Purpose                                                                                                   |
| ----------------------- | --------------------------------------------------------------------------------------------------------- |
| **token.h / token.c**   | Define tokens and their string representations.                                                           |
| **lexer.h / lexer.c**   | Convert source code into tokens; handle comments, literals, and errors.                                   |
| **ast.h / ast.c**       | Define and create nodes for the Abstract Syntax Tree (AST).                                               |
| **parser.h / parser.c** | Parse tokens into structured AST nodes using recursive descent.                                           |
| **value.h / value.c**   | Define runtime value types (`int`, `bool`, `float`, `string`) and implement operations between them.      |
| **env.h / env.c**       | Manage variable scopes, constants, user functions, and built-in functions like `print`.                   |
| **eval.h / eval.c**     | Evaluate AST nodes, execute statements, expressions, and control flow.                                    |
| **run.c**               | The main file-runner — loads `.celer` source files, executes top-level declarations, then calls `main()`. |
| **repl.c**              | Interactive Read–Eval–Print Loop that keeps the environment between inputs.                               |

---

## Example Program

```celer
variable i : int = 0;
const msg : string = "Hola";

Function suma(a : int, b : int) -> int {
  return a + b;
}

Function main() -> void {
  i = suma(10, 32);
  if (i == 42) {
    print("ok, i == ", i);
  } else {
    print("nope");
  }

  for (variable k : int = 0; k < 3; k = k + 1) {
    print("k=", k);
  }

  i = (10 + 2) * 3 == 36 ? { true: 1 : false: 0 };
  print("ternario -> i=", i);
}
```

Output:

```
ok, i == 42
k= 0
k= 1
k= 2
ternario -> i= 1
```

---

## Build Instructions

### Windows (MinGW)

```bat
gcc -std=c99 -Wall -Wextra -O2 -Iinclude src/*.c -o build/celer.exe
```

### Linux / macOS

```bash
cc -std=c99 -Wall -Wextra -O2 -Iinclude src/*.c -o build/celer
```

---

## Running `.celer` Files

**Windows**

```bat
build\celer.exe examples\demo.celer
```

**Linux/macOS**

```bash
./build/celer examples/demo.celer
```

> Only the `main()` function of the file is executed automatically, similar to C.

---

## Using the REPL

### Compile REPL

```bat
gcc -std=c99 -Wall -Wextra -O2 -Iinclude ^
  src/token.c src/lexer.c src/ast.c src/parser.c src/value.c src/env.c src/eval.c src/repl.c ^
  -o build/celer_repl.exe
```

### Run REPL

```bat
build\celer_repl.exe
```

Example session:

```
Celer REPL
Termina cada bloque con ';;'. Comandos: :help, :quit

celer> variable x : int = 41;
celer> ;;
celer> x = x + 1;
celer> ;;
celer> print("x =", x);
celer> ;;
x = 42
celer> Function dup(a : int) -> int { return a + a; }
celer> ;;
celer> print(dup(10));
celer> ;;
20
celer> :quit
Adiós!
```

---

## Error Handling

Celer performs full **lexical and syntactic validation**:

* Missing semicolons → `Se esperaba ';' al final de la sentencia`
* Unclosed parentheses/braces → `Se esperaba ')'` / `Se esperaba '}'`
* Unclosed strings/comments → `Unterminated string/block comment`
* Invalid tokens → `TOK_ILLEGAL`

When a parse error occurs:

* `.celer` files: execution stops and all errors are printed.
* REPL: the chunk is rejected but the session continues.