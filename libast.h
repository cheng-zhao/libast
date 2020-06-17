/*******************************************************************************
* libast.h: this file is part of the libast library.

* libast: C library for evaluating expressions with the abstract syntax tree.

* Github repository:
        https://github.com/cheng-zhao/libast

* Copyright (c) 2020 Cheng Zhao <zhaocheng03@gmail.com>
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*******************************************************************************/

#ifndef _LIBAST_H_
#define _LIBAST_H_

/*============================================================================*\
                            Indicators of variables
\*============================================================================*/

#define AST_VAR_FLAG            '$'
#define AST_VAR_START           '{'
#define AST_VAR_END             '}'


/*============================================================================*\
                         Definitions of data structures
\*============================================================================*/

/* Enumeration of supported data types. */
typedef enum {
  AST_DTYPE_DOUBLE,
  AST_DTYPE_LONG
} ast_dtype_t;

/* The interface of the abstract syntax tree. */
typedef struct {
  ast_dtype_t dtype;    /* Numerical data type for the AST.     */
  int nvar;             /* Number of unique variables.          */
  long *vidx;           /* Unique indices of variables.         */
  void *ast;            /* The root node of the AST.            */
  void *error;          /* Data structure for error handling.   */
} ast_t;


/*============================================================================*\
                            Definitions of functions
\*============================================================================*/

/******************************************************************************
Function `ast_init`:
  Initialise the interface of the abstract syntax tree.
Return:
  The pointer to the interface on success; NULL on error.
******************************************************************************/
ast_t *ast_init(void);

/******************************************************************************
Function `ast_build`:
  Build the abstract syntax tree given the expression and data type.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `str`:      null terminated string for the expression;
  * `dtype`:    data type for the abstract syntax tree.
Return:
  Zero on success; non-zero on error.
******************************************************************************/
int ast_build(ast_t *ast, const char *str, const ast_dtype_t dtype);

/******************************************************************************
Function `ast_eval`:
  Evaluate the value given the abstract syntax tree and the variable array.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `var`:      pointer to the variable array;
  * `size`:     number of elements of the variable array;
  * `value`:    address of the variable holding the evaluated value.
Return:
  Zero on success; non-zero on error.
******************************************************************************/
int ast_eval(ast_t *ast, const void *var, const size_t size, void *value);

/******************************************************************************
Function `ast_perror`:
  Print the error message if there is an error.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `fp`:       output file stream;
  * `msg`:      string to be printed before the error message.
******************************************************************************/
void ast_perror(const ast_t *ast, FILE *fp, const char *msg);

/******************************************************************************
Function `ast_destroy`:
  Release memory allocated for the abstract syntax tree.
Arguments:
  * `ast`:      interface of the abstract syntax tree.
******************************************************************************/
void ast_destroy(ast_t *ast);

#endif
