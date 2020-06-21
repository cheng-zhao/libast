# libast

![GitHub](https://img.shields.io/github/license/cheng-zhao/libast.svg)
![Codacy grade](https://img.shields.io/codacy/grade/416fd62ad6fe4286a7990c79b47a1a6b.svg)

## Table of Contents

-   [Introduction](#introduction)
-   [Expression syntax](#expression-syntax)
    -   [Overview](#overview)
    -   [Data types](#data-types)
    -   [Tokens](#tokens)
-   [Compilation and linking](#compilation-and-linking)
-   [Getting started](#getting-started)
    -   [Initialisation](#initialisation)
    -   [Abstract syntax tree construction](#abstract-syntax-tree-construction)
    -   [Setting variable](#setting-variable)
    -   [Expression evaluation](#expression-evaluation)
    -   [Releasing memory](#releasing-memory)
    -   [Error handling](#error-handling)
-   [Examples](#examples)
    -   [Abstract syntax tree illustration](#abstract-syntax-tree-illustration)
    -   [Parsing the expression in a text file](#parsing-the-expression-in-a-text-file)

## Introduction

This is a portable library written in C, for parsing and evaluating mathematical and logical expressions specified in [infix notation](https://en.wikipedia.org/wiki/Infix_notation). This is done with the [abstract syntax tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree) (AST), constructed using the [shunting-yard algorithm](https://en.wikipedia.org/wiki/Shunting-yard_algorithm). And user-supplied variables are allowed.

This library is compliant with the ISO C99 standard, and relies only on the C standard library. It is written by Cheng Zhao (&#36213;&#25104;), and is distributed under the [MIT license](LICENSE.txt).

<sub>[\[TOC\]](#table-of-contents)</sub>

## Expression syntax

### Overview

The expression supplied to this library has to be in the infix form, e.g.

> (-$2 + sqrt( ${2}**2 - 4 * $1 * $3 )) / (2 * $1)

This expression contains the following types of tokens:
-   <span><code>(&bull;)</code></span>: parenthesis, for overriding the default precedence;
-   `-` (negative sign): unary operator;
-   `+`, `-` (minus sign), `*`, `/`, `**`: binary operators;
-   `$1`, `${2}`: user-supplied variable;
-   `2`, `4`: number literals;
-   <span><code>sqrt(&bull;)</code></span>: function.

In particular, the number following the symbol `$` indicates the index (starting from 1) of the variable in a user-supplied array. And indices with more than one digit have to be enclosed by braces `{}`. The symbols `$`, `{`, and `}` are customisable in [`libast.h`](libast.h#L35).

This library aims at parsing the expression, and evaluating the value given user-supplied variables, with the desired data type.

<sub>[\[TOC\]](#table-of-contents)</sub>

### Data types

The data type of an expression is indicated by its return value. Currently the following data types for expressions are supported: 

| Expression data type          | Indicator          | Native C type   |
|:------------------------------|:------------------:|:---------------:|
| Boolean                       | `AST_DTYPE_BOOL`   | `bool`          |
| Integer                       | `AST_DTYPE_INT`    | `int`           |
| Long integer                  | `AST_DTYPE_LONG`   | `long`          |
| Single precision float number | `AST_DTYPE_FLOAT`  | `float`         |
| Double precision float number | `AST_DTYPE_DOUBLE` | `double`        |

Depending on the data type of the expression, number literals are parsed as the corresponding type accordingly. For instance, number literals for an `AST_DTYPE_DOUBLE` type expression are all read as double-precision floating-point numbers. For boolean expressions, however, numbers are only parsed as `long` &mdash; if applicable &mdash; or `double` types. String literals must be enclosed by single (`'`) or double (`"`) quotation marks, and are only allowed for boolean expressions.

The allowed data type for a user-supplied variable depends also on the data type of the expression are listed below. For numerical expressions, variables with a different data type are casted to the expression type whenever possible. And for boolean expressions, integer and floating-point variables are converted to the `long` and `double` types, respectively.
| Variable data type                          | Indicator          | Native C type               | Valid for expression type                                                                            |
|:--------------------------------------------|:------------------:|:---------------------------:|------------------------------------------------------------------------------------------------------|
| Boolean                                     | `AST_DTYPE_BOOL`   | `bool`                      | `AST_DTYPE_BOOL`                                                                                     |
| Integer                                     | `AST_DTYPE_INT`    | `int`                       | `AST_DTYPE_BOOL`,<br />`AST_DTYPE_INT`, `AST_DTYPE_LONG`,<br />`AST_DTYPE_FLOAT`, `AST_DTYPE_DOUBLE` |
| Long integer                                | `AST_DTYPE_LONG`   | `long`                      | `AST_DTYPE_BOOL`, `AST_DTYPE_LONG`,<br />`AST_DTYPE_FLOAT`, `AST_DTYPE_DOUBLE`                       |
| Single-precision<br />floating-point number | `AST_DTYPE_FLOAT`  | `float`                     | `AST_DTYPE_BOOL`,<br />`AST_DTYPE_FLOAT`, `AST_DTYPE_DOUBLE`                                         |
| Double-precision<br />floating-point number | `AST_DTYPE_DOUBLE` | `double`                    | `AST_DTYPE_BOOL`, `AST_DTYPE_DOUBLE`                                                                 |
| String                                      | `AST_DTYPE_STRING` | `char *`<br />with a length | `AST_DTYPE_BOOL`                                                                                     |

For convenience we define some mixture data types, that are helpful for indicating the data types for operators. Note that these types are only for explanation purposes. Though they are used internally in this library, it is invalid to pass them to the interfaces.

| Mixture data type   | Meaning                                  |
|:-------------------:|:----------------------------------------:|
| `AST_DTYPE_INTEGER` | `AST_DTYPE_INT` or `AST_DTYPE_LONG`      |
| `AST_DTYPE_REAL`    | `AST_DTYPE_FLOAT` or `AST_DTYPE_DOUBLE`  |
| `AST_DTYPE_NUMBER`  | `AST_DTYPE_INTEGER` or `AST_DTYPE_REAL`  |
| `AST_DTYPE_NATIVE`  | `AST_DTYPE_BOOL` or `AST_DTYPE_NUMBER`   |
| `AST_DTYPE_ALL`     | `AST_DTYPE_NATIVE` or `AST_DTYPE_STRING` |

<sub>[\[TOC\]](#table-of-contents)</sub>

### Tokens

A full list of the supported tokens, as well as their attributes, are listed below. Note that only the unary operators `-` (negative), `!` (logical not), and `~` (bitwise not) are evaluated _right-to-left_, while the associativity for all the other tokens are _left-to-right_.

| Token                                                                     | Description                                     | Precedence | Accepted data type  | Return type         |
|:-------------------------------------------------------------------------:|:-----------------------------------------------:|:----------:|:-------------------:|:-------------------:|
| Literals                                                                  | Number or string                                | &mdash;    | &mdash;             | &mdash;             |
| <span><code>&#36;&bull;</code></span>/<span><code>${&bull;}</code></span> | Variable                                        | &mdash;    | &mdash;             | &mdash;             |
| <span><code>(&bull;)</code></span>                                        | Parenthesis                                     | &mdash;    | &mdash;             | &mdash;             |
| <span><code>abs(&bull;)</code></span>                                     | Absolute value                                  | &mdash;    | `AST_DTYPE_NUMBER`  | `AST_DTYPE_NUMBER`  |
| <span><code>sqrt(&bull;)</code></span>                                    | Square root                                     | &mdash;    | `AST_DTYPE_NUMBER`  | `AST_DTYPE_REAL`    |
| <span><code>ln(&bull;)</code></span>                                      | Natural logarithm                               | &mdash;    | `AST_DTYPE_NUMBER`  | `AST_DTYPE_REAL`    |
| <span><code>log(&bull;)</code></span>                                     | Base 10 logarithm                               | &mdash;    | `AST_DTYPE_NUMBER`  | `AST_DTYPE_REAL`    |
| `-`                                                                       | Negative                                        | 12         | `AST_DTYPE_NUMBER`  | `AST_DTYPE_NUMBER`  |
| `!`                                                                       | Logical NOT                                     | 12         | `AST_DTYPE_NATIVE`  | `AST_DTYPE_BOOL`    |
| `~`                                                                       | Bitwise NOT                                     | 12         | `AST_DTYPE_INTEGER` | `AST_DTYPE_INTEGER` |
| `**`                                                                      | Exponent                                        | 11         | `AST_DTYPE_NUMBER`  | `AST_DTYPE_NUMBER`  |
| `*`, `/`, `%`                                                             | Multiplication, division,<br />and remainder    | 10         | `AST_DTYPE_NUMBER`  | `AST_DTYPE_NUMBER`  |
| `+`, `-`                                                                  | Addition and subtraction                        | 9          | `AST_DTYPE_NUMBER`  | `AST_DTYPE_NUMBER`  |
| `<<`, `>>`                                                                | Bitwise left shift<br />and right shift         | 8          | `AST_DTYPE_INTEGER` | `AST_DTYPE_INTEGER` |
| `<`, `<=`, `>`, `>=`                                                      | Relational &lt;, &le;, &gt;, &ge;               | 7          | `AST_DTYPE_NUMBER`  | `AST_DTYPE_BOOL`    |
| `==`, `!=`                                                                | Relational = and &ne;                           | 6          | `AST_DTYPE_ALL`     | `AST_DTYPE_BOOL`    |
| `&`                                                                       | Bitwise AND                                     | 5          | `AST_DTYPE_INTEGER` | `AST_DTYPE_INTEGER` |
| `^`                                                                       | Bitwise XOR                                     | 4          | `AST_DTYPE_INTEGER` | `AST_DTYPE_INTEGER` |
| <span><code>&#124;</code></span>                                          | Bitwise OR                                      | 3          | `AST_DTYPE_INTEGER` | `AST_DTYPE_INTEGER` |
| `&&`                                                                      | Logical AND                                     | 2          | `AST_DTYPE_BOOL`    | `AST_DTYPE_BOOL`    |
| <span><code>&#124;&#124;</code></span>                                    | Logical OR                                      | 1          | `AST_DTYPE_BOOL`    | `AST_DTYPE_BOOL`    |

<sub>[\[TOC\]](#table-of-contents)</sub>

## Compilation and linking

Since this library is small and portable, it is recommended to compile the only two source files &mdash; `libast.h` and `libast.c` &mdash; along with your own program. To this end, one only needs to include the header `libast.h` in the source file for parsing configurations:

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
  ast_dtype_t dtype;    /* Data type for the expression.        */
  long nvar;            /* Number of unique variables.          */
  void *var;            /* The list of unique variables.        */
  long *vidx;           /* Unique indices of variables.         */
  char *exp;            /* A copy of the expression string.     */
  void *ast;            /* The root node of the AST.            */
  void *error;          /* Data structure for error handling.   */
} ast_t;
```

<sub>[\[TOC\]](#table-of-contents)</sub>

### Abstract syntax tree construction

To parse an expression, one needs only to call the function

```c
int ast_build(ast_t *ast, const char *str, const ast_dtype_t dtype, const bool eval);
```

The arguments are:
-   `ast`: the interface initialised by `ast_init`;
-   `str`: the string for the input expression;
-   `dtype`: one of the pre-defined data type: `AST_DTYPE_BOOL`, `AST_DTYPE_INT`, `AST_DTYPE_LONG`, `AST_DTYPE_FLOAT`, or `AST_DTYPE_DOUBLE`;
-   `eval`: if it is `true`, then pre-compute values for operators that are supplied only numerical literals.

This function returns `0` on success, and a non-zero integer on error. Apart from the construction of the AST, it sets also the members `nvar` and `vidx` of the interface, which are the number of unique variables specified in the expression, as well as their indices, respectively.

Note that one instance of the `ast_t` type interface can only be used once for a single expression. To parse another expression, a new interface has to be initialised (see [Initialisation](#initialisation)).

<sub>[\[TOC\]](#table-of-contents)</sub>

### Setting variable

Variable values can be passed to the AST using the function

```c
int ast_set_var(ast_t *ast, const long idx, const void *value, const size_t size,
    const ast_dtype_t dtype);
```

The arguments are
-   `ast`: the interface of the AST;
-   `idx`: index of the variable (starting from 1);
-   `value`: pointer to a variable holding the value to be set;
-   `size`: number of characters (without the `'\0'` for termination), if this variable indicates a string;
-   `dtype`: datatype of this variable: `AST_DTYPE_BOOL`, `AST_DTYPE_INT`, `AST_DTYPE_LONG`, `AST_DTYPE_FLOAT`, `AST_DTYPE_DOUBLE`, or `AST_DTYPE_STRING`.

This function returns `0` on success, and a non-zero integer on error. It sets only one variable for once, and performs type casting if necessary. This is relatively inefficient if the user has already variables with the same data type as the expression, and would like to supply all of them at once. Therefore, this way of setting variables is not necessary in some cases for non-boolean-type expressions.

<sub>[\[TOC\]](#table-of-contents)</sub>

### Expression evaluation

The function for expression evaluation is

```c
int ast_eval(ast_t *ast, void *value);
```

Here, `ast` indicates the interface of the AST, and `value` denotes the address of a variable for storing the result. Note that the data type of this variable has to be identical with the data type of the expression, which is specified for the AST construction.

If the data type of the expression is numerical (non-boolean), the evaluation can be done with the following function, provided that the user-supplied variables are all in the same data type as the expressions, and can be passed as an array:

```c
int ast_eval_num(ast_t *ast, void *value, const void *var, const size_t size);
```

Here, `var` denotes the array of the user-supplied variables, with `size` being the total number of elements. In particular, the array index of an variable has to be one less than the variable index set in the expression. For instance, the variable `$3` must be the 3rd element in the array, i.e., with the array index of `2`, since array indexing in C starts from 0.

Both `ast_eval` and `ast_eval_num` return `0` on success, and an non-zero integer on failure. Furthermore, function `ast_eval_num` is thread-safe.

<sub>[\[TOC\]](#table-of-contents)</sub>

### Releasing memory

If an expression is not going to be used anymore, the corresponding interface needs to be deconstructed using the function

```c
void ast_destroy(ast_t *ast);
```

<sub>[\[TOC\]](#table-of-contents)</sub>

### Error handling

Errors can be caught by checking the return values of some of the functions, such as `ast_init`, `ast_build`, `ast_set_var`, and `ast_eval`. The corresponding error messages can always be printed using the function

```c
void cfg_perror(const cfg_t *cfg, FILE *stream, const char *msg);
```

It outputs the string indicated by `msg`, followed by a colon and a space, and then followed by the error message produced by this library, as well as a newline character `\n`. The results are written to `stream`, which is typically `stderr`.

<sub>[\[TOC\]](#table-of-contents)</sub>

## Examples

Two examples on the usage of this library are provided in the [example](example) folder. By default they can both be compiled with the command

```console
make
```

### Abstract syntax tree illustration

The file [`draw_tree.c`](example/draw_tree.c) is an implementation of the AST illustration with ASCII characters and ANSI colours. By default the filename of the compiled executable is `libast_draw`. It should be called with two command line options, the first indicating the data type of the expression (can be `BOOL`, `INT`, `LONG`, `FLOAT`, and `DOUBLE`), followed by the expression.

For instance, the AST constructed for the expression shown in the [Overview section](#overview) can be drawn by this example:

```console
$ ./libast_draw DOUBLE '(-$2 + sqrt(${2}**2 - 4*$1*$3)) / (2*$1)'
/
|-- +
|   |-- -
|   |   `-- $2
|   `-- sqrt
|       `-- -
|           |-- **
|           |   |-- $2
|           |   `-- 2
|           `-- *
|               |-- *
|               |   |-- 4
|               |   `-- $1
|               `-- $3
`-- *
    |-- 2
    `-- $1
```

<sub>[\[TOC\]](#table-of-contents)</sub>

### Parsing the expression in a text file

The file [`parse_file.c`](example/parse_file.c) is for parsing a text file with the definitions of the expression and variables. The text file should be in the following format

```console
EXP_DATA_TYPE EXPRESSION

VAR1_DATA_TYPE VAR1_INDEX VAR1_VALUE
VAR2_DARA_TYPE VAR2_INDEX VAR2_VALUE
...
```

Here `EXP_DATA_TYPE` indicates the data type of the expression, and should be `BOOL`, `INT`, `LONG`, `FLOAT`, or `DOUBLE`. `EXPRESSION` denotes the string for the expression. `VARn_DATA_DTYPE` indicates the data type for the `n`-th variable, which can be `BOOL`, `INT`, `LONG`, `FLOAT`, and `STRING`. `VARn_INDEX` and `VARn_VALUE` are the index and the value for this variable, respectively. In particular, the value of boolean type variables must be specified as `1` (`true`) or `0` (`false`) in this file. Note however that this is only the requirement for this example, instead of the limitation of the library.

An example of the text file, [`input.txt`](example/input.txt),  is also provided in the [example](example) folder:

```console
$ cat input.txt
DOUBLE (-$2 + sqrt( ${2}**2 - 4 * $1 * $3 )) / (2 * $1)
INT 1 1
DOUBLE 2 6
FLOAT 3 5
```

By default the executable for this example is `libast_parse`, which should be called followed by the name of the file to be parsed, e.g.

```console
$ ./libast_parse input.txt
Expression:  (-$2 + sqrt( ${2}**2 - 4 * $1 * $3 )) / (2 * $1)

Variable ${1} (INT): 1
Variable ${2} (DOUBLE): 6
Variable ${3} (FLOAT): 5
Result: -1
```

<sub>[\[TOC\]](#table-of-contents)</sub>
