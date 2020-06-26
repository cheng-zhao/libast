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
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "libast.h"

/* Print the error message and exit. */
#define PRINT_ERROR(ast) {                              \
  ast_perror(ast, stderr, "\x1B[31;1mError:\x1B[0m");   \
  ast_destroy(ast);                                     \
  return 1;                                             \
}

/* Maximum number of characters read per line. */
#define BUF 512

/* Read the data type indicator from the input string. */
int get_type(char *line, char **end) {
    if (!strncmp(line, "BOOL", 4)) {
      *end = line + 4;
      return AST_DTYPE_BOOL;
    }
    else if (!strncmp(line, "INT", 3)) {
      *end = line + 3;
      return AST_DTYPE_INT;
    }
    else if (!strncmp(line, "LONG", 4)) {
      *end = line + 4;
      return AST_DTYPE_LONG;
    }
    else if (!strncmp(line, "FLOAT", 5)) {
      *end = line + 5;
      return AST_DTYPE_FLOAT;
    }
    else if (!strncmp(line, "DOUBLE", 6)) {
      *end = line + 6;
      return AST_DTYPE_DOUBLE;
    }
    else if (!strncmp(line, "STRING", 6)) {
      *end = line + 6;
      return AST_DTYPE_STRING;
    }
    else return -1;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s filename\n", argv[0]);
    return 1;
  }

  ast_t *ast = ast_init();
  if (!ast) PRINT_ERROR(ast);

  FILE *fp;
  char line[BUF];
  char strv[BUF];
  bool ast_built = false;

  bool bv;
  int iv;
  long lv;
  float fv;
  double dv;

  if (!(fp = fopen(argv[1], "r"))) {
    fprintf(stderr, "Error: cannot open file %s\n", argv[1]);
    return 1;
  }

  /* Start reading the file. */
  memset(line, 0, BUF);
  memset(strv, 0, BUF);
  char *s = strv;
  while (fgets(line, BUF - 1, fp) != NULL) {
    char *tmp = line;
    while (isspace(*tmp)) tmp++;        /* omit white spaces */
    int type = get_type(tmp, &tmp);
    if (type < 0) continue;
    if (!ast_built) {                   /* build the abstract syntax tree */
      if (type == AST_DTYPE_STRING) continue;
      if (ast_build(ast, tmp, type, true)) PRINT_ERROR(ast);
      ast_built = true;
      printf("Expression: %s\n", tmp);
      continue;
    }

    /* Read the variable. */
    long id = 0;
    int pos = 0;
    sscanf(tmp, "%ld%n", &id, &pos);
    if (!pos) continue;
    char *str = tmp + pos;

    switch (type) {
      case AST_DTYPE_BOOL:
        iv = -1;
        sscanf(str, "%d", &iv);
        if (iv == 0) bv = false;
        else if (iv == 1) bv = true;
        else {
          fprintf(stderr, "Unrecogised BOOL: %s\n", tmp);
          return 1;
        }
        if (ast_set_var(ast, id, &bv, 0, AST_DTYPE_BOOL)) PRINT_ERROR(ast);
        printf("Variable %c%c%ld%c (BOOL): ",
            AST_VAR_FLAG, AST_VAR_START, id, AST_VAR_END);
        if (bv) printf("TRUE\n");
        else printf("FALSE\n");
        break;
      case AST_DTYPE_INT:
        sscanf(str, "%d", &iv);
        if (ast_set_var(ast, id, &iv, 0, AST_DTYPE_INT)) PRINT_ERROR(ast);
        printf("Variable %c%c%ld%c (INT): %d\n",
            AST_VAR_FLAG, AST_VAR_START, id, AST_VAR_END, iv);
        break;
      case AST_DTYPE_LONG:
        sscanf(str, "%ld", &lv);
        if (ast_set_var(ast, id, &lv, 0, AST_DTYPE_LONG)) PRINT_ERROR(ast);
        printf("Variable %c%c%ld%c (LONG): %ld\n",
            AST_VAR_FLAG, AST_VAR_START, id, AST_VAR_END, lv);
        break;
      case AST_DTYPE_FLOAT:
        sscanf(str, "%f", &fv);
        if (ast_set_var(ast, id, &fv, 0, AST_DTYPE_FLOAT)) PRINT_ERROR(ast);
        printf("Variable %c%c%ld%c (FLOAT): %.8g\n",
            AST_VAR_FLAG, AST_VAR_START, id, AST_VAR_END, fv);
        break;
      case AST_DTYPE_DOUBLE:
        sscanf(str, "%lf", &dv);
        if (ast_set_var(ast, id, &dv, 0, AST_DTYPE_DOUBLE)) PRINT_ERROR(ast);
        printf("Variable %c%c%ld%c (DOUBLE): %.12g\n",
            AST_VAR_FLAG, AST_VAR_START, id, AST_VAR_END, dv);
        break;
      case AST_DTYPE_STRING:
        if (sscanf(str, "%s", s) != 1) {
          fprintf(stderr, "Error: failed to read the string.\n");
          return 1;
        }
        iv = strlen(s);
        if (ast_set_var(ast, id, s, iv, AST_DTYPE_STRING)) PRINT_ERROR(ast);
        printf("Variable %c%c%ld%c (STRING): %.*s\n",
            AST_VAR_FLAG, AST_VAR_START, id, AST_VAR_END, iv, s);
        s += iv + 1;
        if (s >= strv + BUF) {
          fprintf(stderr, "Error: the strings are too long.\n");
          return 1;
        }
        break;
      default:
        break;
    }
    memset(line, 0, BUF);
  }
  fclose(fp);

  /* Evaluation. */
  switch (ast->dtype) {
    case AST_DTYPE_BOOL:
      bv = false;
      if (ast_eval(ast, &bv)) PRINT_ERROR(ast);
      if (bv) printf("Result: TRUE\n");
      else printf("Result: FALSE\n");
      break;
    case AST_DTYPE_INT:
      if (ast_eval(ast, &iv)) PRINT_ERROR(ast);
      printf("Result: %d\n", iv);
      break;
    case AST_DTYPE_LONG:
      if (ast_eval(ast, &lv)) PRINT_ERROR(ast);
      printf("Result: %ld\n", lv);
      break;
    case AST_DTYPE_FLOAT:
      if (ast_eval(ast, &fv)) PRINT_ERROR(ast);
      printf("Result: %.8g\n", fv);
      break;
    case AST_DTYPE_DOUBLE:
      if (ast_eval(ast, &dv)) PRINT_ERROR(ast);
      printf("Result: %.12g\n", dv);
      break;
    default:
      break;
  }

  ast_destroy(ast);
  return 0;
}
