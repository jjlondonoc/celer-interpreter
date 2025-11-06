# Celer — Intérprete en C

**Celer** es un lenguaje interpretado, tipado estáticamente e inspirado en C, implementado completamente en **C99**.
Fue creado para explorar cómo se puede construir un lenguaje desde cero —desde el analizador léxico y el parser hasta el AST y el evaluador— manteniendo un diseño simple, eficiente y educativo.

---

## Filosofía

> *“Con espíritu de C: control, eficiencia y ejecución cercana al metal.”*

Celer ofrece:

* Tipado estático con un conjunto reducido de tipos primitivos.
* Sintaxis familiar para cualquiera que conozca C.
* Arquitectura modular y clara: `token → lexer → parser → AST → evaluator → REPL`.
* Totalmente escrito en C99 portátil y sin dependencias externas.

---

## Especificación del Lenguaje

### Tipos de Datos

| Tipo     | Descripción             | Ejemplo(s)             |
| -------- | ----------------------- | ---------------------- |
| `int`    | Entero                  | `0`, `-12`, `42`       |
| `float`  | Número en coma flotante | `3.14`, `-0.5`, `10.0` |
| `bool`   | Booleano                | `true`, `false`        |
| `string` | Texto entre comillas    | `"hola"`, `"linea\n"`  |

---

### Literales y Secuencias de Escape

Las cadenas (`string`) soportan secuencias de escape:

| Escape | Significado     |
| :----- | :-------------- |
| `\n`   | Nueva línea     |
| `\t`   | Tabulación      |
| `\\`   | Barra invertida |
| `\"`   | Comilla doble   |

Ejemplo:

```celer
const msg : string = "línea1\nlínea2\tTAB\\slash\"quote";
```

---

### Comentarios

```celer
*-- Comentario de una línea

/* Comentario
   de varias líneas */
```

---

### Variables y Constantes

Cada declaración debe terminar con `;`.

```celer
variable x : int = 10;
const saludo : string = "hola";
```

`variable` define un valor mutable.
`const` define una constante inmutable.

---

### Funciones

Definidas con la palabra reservada **Function** (con F mayúscula).

```celer
Function suma(a : int, b : int) -> int {
  return a + b;
}
```

* Los parámetros usan la notación `nombre : tipo`.
* El tipo de retorno se indica después de `->`.
* Las funciones pueden declararse en cualquier orden.

---

### Control de Flujo

#### **if / else**

```celer
if (x > 0) {
  print("Positivo");
} else {
  print("No positivo");
}
```

#### **for**

Celer soporta dos formas de `for`:

**Estilo while:**

```celer
for (x < 10) {
  print(x);
  x = x + 1;
}
```

**Estilo C:**

```celer
for (variable i : int = 0; i < 3; i = i + 1) {
  print("i =", i);
}
```

Soporta `break` y `continue`.

---

### Operador Ternario

Forma estructurada especial:

```celer
condición ? { true: expresión_si_true : false: expresión_si_false };
```

Ejemplo:

```celer
variable y : int = x > 5 ? { true: 1 : false: 0 };
```

---

### Operadores

| Categoría   | Operadores                                 |   |        |
| :---------- | :----------------------------------------- | - | ------ |
| Aritméticos | `+`, `-`, `*`, `/`, `%`                    |   |        |
| Comparación | `==`, `!=`, `<`, `<=`, `>`, `>=`           |   |        |
| Lógicos     | `&&`, `                                    |   | `, `!` |
| Asignación  | `=`, `+=`, `-=`, `*=`, `/=`, `%=`          |   |        |
| Otros       | `()`, `{}`, `[]`, `,`, `;`, `:`, `?`, `->` |   |        |

---

### Funciones Integradas

| Función      | Descripción                                                                       |
| ------------ | --------------------------------------------------------------------------------- |
| `print(...)` | Imprime todos los argumentos separados por espacios y termina con salto de línea. |

Ejemplo:

```celer
print("hola", 42, true);
```

Salida:

```
hola 42 true
```

---

## Estructura del Proyecto

```
celer/
├── include/
│   ├── token.h      # Definición de tipos de token
│   ├── lexer.h      # Analizador léxico
│   ├── ast.h        # Árbol de sintaxis abstracta
│   ├── parser.h     # Parser de descenso recursivo
│   ├── value.h      # Representación de valores en tiempo de ejecución
│   ├── env.h        # Entorno (variables, funciones, builtins)
│   ├── eval.h       # Evaluador / intérprete
│
├── src/
│   ├── token.c
│   ├── lexer.c
│   ├── ast.c
│   ├── parser.c
│   ├── value.c
│   ├── env.c
│   ├── eval.c
│   ├── run.c        # Ejecuta archivos .celer (runner principal)
│   └── repl.c       # REPL interactivo
│
├── examples/
│   ├── demo.celer   # Ejemplo completo
│   └── mini.celer   # Ejemplo mínimo
│
├── build/           # Binarios compilados (ignorados en Git)
└── README.md
```

---

## Descripción de Archivos

| Archivo                 | Propósito                                                                         |
| ----------------------- | --------------------------------------------------------------------------------- |
| **token.h / token.c**   | Define los tokens y sus representaciones.                                         |
| **lexer.h / lexer.c**   | Convierte texto fuente en tokens, maneja comentarios y literales.                 |
| **ast.h / ast.c**       | Define los nodos del Árbol de Sintaxis Abstracta (AST).                           |
| **parser.h / parser.c** | Analiza los tokens y construye el AST.                                            |
| **value.h / value.c**   | Define los tipos de valores en tiempo de ejecución y las operaciones entre ellos. |
| **env.h / env.c**       | Maneja entornos, variables, constantes, funciones y builtins.                     |
| **eval.h / eval.c**     | Evalúa el AST, ejecuta el flujo de control y las expresiones.                     |
| **run.c**               | Carga y ejecuta archivos `.celer`, llamando automáticamente a `main()`.           |
| **repl.c**              | Proporciona un REPL interactivo persistente.                                      |

---

## Ejemplo de Programa

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

Salida:

```
ok, i == 42
k= 0
k= 1
k= 2
ternario -> i= 1
```

---

## Compilación

### Windows (MinGW)

```bat
gcc -std=c99 -Wall -Wextra -O2 -Iinclude src/*.c -o build/celer.exe
```

### Linux / macOS

```bash
cc -std=c99 -Wall -Wextra -O2 -Iinclude src/*.c -o build/celer
```

---

## Ejecución de Archivos `.celer`

**Windows**

```bat
build\celer.exe examples\demo.celer
```

**Linux/macOS**

```bash
./build/celer examples/demo.celer
```

> Solo se ejecuta la función `main()` del archivo, al igual que en C.

---

## Uso del REPL

### Compilar REPL

```bat
gcc -std=c99 -Wall -Wextra -O2 -Iinclude ^
  src/token.c src/lexer.c src/ast.c src/parser.c src/value.c src/env.c src/eval.c src/repl.c ^
  -o build/celer_repl.exe
```

### Ejecutar REPL

```bat
build\celer_repl.exe
```

Ejemplo de sesión:

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

## Manejo de Errores

Celer realiza validación **léxica y sintáctica completa**:

* Falta de punto y coma → `Se esperaba ';' al final de la sentencia`
* Paréntesis o llaves sin cerrar → `Se esperaba ')'` / `Se esperaba '}'`
* Cadenas o comentarios sin cerrar → `Unterminated string/block comment`
* Tokens inválidos → `TOK_ILLEGAL`

En caso de error de parseo:

* En archivos `.celer`: se muestran los errores y se detiene la ejecución.
* En el REPL: el bloque se descarta y la sesión continúa.

