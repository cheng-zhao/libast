# libast

![GitHub](https://img.shields.io/github/license/cheng-zhao/libast.svg)


## Table of Contents

-   [Introduction](#introduction)
-   [Expression syntax](#expression-syntax)
-   [Compilation and linking](#compilation-and-linking)
-   [Getting started](#getting-started)
    -   [Initialisation](#initialisation)
    -   [Abstract syntax tree construction](#abstract-syntax-tree-construction)
    -   [Expression evaluation](#expression-evaluation)
    -   [Releasing memory](#releasing-memory)
    -   [Error handling](#error-handling)
    -   [Examples](#examples)

## Introduction

This is a simple library written in C, for parsing and evaluating mathematical and logical expressions specified in [infix notation](https://en.wikipedia.org/wiki/Infix_notation). This is done with the [abstract syntax tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree) (AST), constructed using the [Shunting-yard algorithm](https://en.wikipedia.org/wiki/Shunting-yard_algorithm). And user-supplied variables are allowed.

This library is compliant with the ISO C99 standard, and relies only on the C standard library. It is written by Cheng Zhao (&#36213;&#25104;), and is distributed under the [MIT license](LICENSE.txt).

<sub>[\[TOC\]](#table-of-contents)</sub>

## Expression syntax

The expression supplied to this library has to be in the infix form, e.g.

> (-$2 + sqrt( ${2}^2 - 4 * $1 * $3 )) / (2 * $1)

This expression contains the following types of tokens:
-   <span><code>(&bull;)</code></span>: parenthesis, for override the default precedence;
-   `-` (negative sign): unary operator;
-   `+`, `-` (minus sign), `*`, `/`, `^`: binary operators;
-   `$1`, `${2}`: user-supplied variable;
-   `2`, `4`: numbers;
-   <span><code>sqrt(&bull;)</code></span>: function.

In particular, the number following the symbol `$` indicates the index (starting from 1) of the variable in a user-supplied array. And indices with more than one digit have to be enclosed by braces `{}`. The symbols `$`, `{`, and `}` are customisable in [`libast.h`](libast.h#L35).

Currently two data types are supported for the evaluations:
-   `AST_DTYPE_DOUBLE`: for double-precision floating-point numbers;
-   `AST_DTYPE_LONG`: for long integer numbers.

And for logical expressions, the number `1` is returned if the expression is `true`, while `0` represents `false`. 

A full list of supported tokens, as well as their attributes, are listed below. Note that currently only the _left-to-right_ associativity is implemented in this library.

| Token                                                                        | Description       | Precedence | Number of arguments |
|------------------------------------------------------------------------------|-------------------|------------|---------------------|
| Numbers                                                                      | Numbers           | &mdash;    | &mdash;             |
| <span><code>&#36;&bull;</code></span>/<span><code>${&bull;}</code></span>    | Variables         | &mdash;    | &mdash;             |
| <span><code>(&bull;)</code></span>                                           | Parenthesis       | &mdash;    | &mdash;             |
| <span><code>sqrt(&bull;)</code></span>                                       | Square root       | 9          | 1                   |
| <span><code>ln(&bull;)</code></span>                                         | natural logarithm | 9          | 1                   |
| <span><code>log(&bull;)</code></span>                                        | Base 10 logarithm | 9          | 1                   |
| `-`                                                                          | Negative          | 8          | 1                   |
| `!`                                                                          | Logical NOT       | 7          | 1                   |
| `^`                                                                          | Exponent          | 6          | 2                   |
| `*`                                                                          | Multiplication    | 5          | 2                   |
| `/`                                                                          | Division          | 5          | 2                   |
| `+`                                                                          | Addition          | 4          | 2                   |
| `-`                                                                          | Subtraction       | 4          | 2                   |
| `>`                                                                          | Relational &gt;   | 3          | 2                   |
| `>=`                                                                         | Relational &ge;   | 3          | 2                   |
| `<`                                                                          | Relational &lt;   | 3          | 2                   |
| `<=`                                                                         | Relational &le;   | 3          | 2                   |
| `==`                                                                         | Relational =      | 2          | 2                   |
| `!=`                                                                         | Relational &ne;   | 2          | 2                   |
| `&&`                                                                         | Logical AND       | 1          | 2                   |
| `||`                                                                         | Logical OR        | 0          | 2                   |

<sub>[\[TOC\]](#table-of-contents)</sub>

## Compilation and linking

Since this library is tiny and portable, it is recommended to compile the only two source files &mdash; `libast.h` and `libast.c` &mdash; along with your own program. To this end, one only needs to include the header `libast.h` in the source file for parsing configurations:

```c
#include "libast.h"
```

<sub>[\[TOC\]](#table-of-contents)</sub>

## Getting started

### Initialisation

Before parsing an expression, the interface of the AST should be initialised using the function `ast_init`, e.g.

```c
ast_t *ast = ast_init();
```

This function returns `NULL` on error. And the `ast_t` type interface is defined as

```c
typedef struct {
  ast_dtype_t dtype;    /* Numerical data type for the AST.     */
  int nvar;             /* Number of unique variables.          */
  long *vidx;           /* Unique indices of variables.         */
  void *ast;            /* The root node of the AST.            */
  void *error;          /* Data structure for error handling.   */
} ast_t;
```

<sub>[\[TOC\]](#table-of-contents)</sub>

### Abstract syntax tree construction

To parse an expression, one needs only to call the function

```c
int ast_build(ast_t *ast, const char *str, const ast_dtype_t dtype);
```

The arguments are:
-   `ast`: the interface initialised by `ast_init`;
-   `str`: the string for the input expression;
-   `dtype`: one of the pre-defined data type: `AST_DTYPE_DOUBLE` or `AST_DTYPE_LONG`.

This function returns `0` on success, and a non-zero integer on error. Apart from the construction of the AST, it sets also the members `nvar` and `vidx` of the interface, which are the number of unique variables specified in the expression, as well as their indices, respectively.

Note that one instance of the `ast_t` type interface can only be used once for a single expression. To parse another expression, a new interface has to be initialised (see [Initialisation](#initialisation)).

<sub>[\[TOC\]](#table-of-contents)</sub>

### Expression evaluation

The function for expression evaluation is

```c
int ast_eval(ast_t *ast, const void *var, const size_t size, void *value);
```

And the arguments are
-   `ast`: the interface of the AST;
-   `var`: the user-supplied variable array;
-   `size`: length of the variable array;
-   `value`: address of a variable for storing the result.

This function returns `0` on success, and a non-zero integer on error. Note that `value` has to be the address of either a `double` or a `long` variable, depending on the data type for the AST construction.

<sub>[\[TOC\]](#table-of-contents)</sub>

### Releasing memory

If an expression is not going to be used anymore, the corresponding interface needs to be deconstructed using the function

```c
void ast_destroy(ast_t *ast);
```

<sub>[\[TOC\]](#table-of-contents)</sub>

### Error handling

Errors can be caught by checking the return values of some of the functions, such as `ast_init`, `ast_build`, and `ast_eval`. The corresponding error messages can always be printed using the function

```c
void cfg_perror(const cfg_t *cfg, FILE *stream, const char *msg);
```

It outputs the string indicated by `msg`, followed by a colon and a space, and then followed by the error message produced by this library, as well as a newline character `\n`. The results are written to `stream`, which is typically `stderr`.

<sub>[\[TOC\]](#table-of-contents)</sub>

### Examples

An example for the usage of this library is provided in the [example](example) folder. It parses an expression passed via command line arguments, and evaluates the value with the pre-defined variable array.

<sub>[\[TOC\]](#table-of-contents)</sub>
