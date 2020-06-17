/*******************************************************************************
* example.c: this file is an example for the usage of the libcfg library.

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

#include <stdio.h>
#include "libast.h"

/* Pre-defined data type and variable array. */
typedef double dtype;
#define DTYPE AST_DTYPE_DOUBLE
#define NUM 5
#define ARRAY {1, 1e-2, 3.14, -1e99}
#define FMT "%lg"

/* Print the error message and exit. */
#define PRINT_ERROR(ast) {                              \
  ast_perror(ast, stderr, "\x1B[31;1mError:\x1B[0m");   \
  ast_destroy(ast);                                     \
  return 1;                                             \
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s EXPRESSION\n", argv[0]);
    return 1;
  }

  dtype var[NUM] = ARRAY;
  dtype res;

  ast_t *ast = ast_init();
  if (!ast) PRINT_ERROR(ast);
    
  if (ast_build(ast, argv[1], DTYPE)) PRINT_ERROR(ast);
  if (ast_eval(ast, var, NUM, &res)) PRINT_ERROR(ast);

  printf("Expression: '%s'\n", argv[1]);
  printf("Variables:");
  for (int i = 0; i < NUM; i++) printf(" " FMT, var[i]);
  printf("\nUsed variables:");
  for (int i = 0; i < ast->nvar; i++) printf(" " FMT, var[ast->vidx[i]]);
  printf("\nResult: " FMT "\n", res);

  ast_destroy(ast);
  return 0;
}

