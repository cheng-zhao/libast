/*******************************************************************************
* print_tree.c: this file is for drawing the abstract syntax tree.

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
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "libast.h"

/* Tagged union for variables with different data types. */
typedef struct {
  int dtype;
  union {
    bool bval; int ival; long lval; float fval; double dval;
    struct ast_string_struct_t { size_t len; char *str; } sval;
  } v;
} ast_var_t;

/* The abstract syntax tree (AST). */
typedef struct ast_tree_struct {
  int type;                             /* type of the token           */
  ast_var_t value;                      /* value of the token          */
  const char *ptr;                      /* poisition in the expression */
  struct ast_tree_struct *parent;       /* parent node                 */
  struct ast_tree_struct *left;         /* left child node             */
  struct ast_tree_struct *right;        /* right child node            */
} ast_node_t;

/* Symbols for the tokens. */
const char *token[] = { "NULL", "NUM", "STR", "VAR", "(", ")", "abs", "sqrt",
  "ln", "log", "-", "!", "~", "**", "*", "/", "%", "+", "-", "<<", ">>",
  "<", "<=", ">", ">=", "==", "!=", "&", "^", "|", "&&", "||" };

/* Print the error message and exit. */
#define PRINT_ERROR(ast) {                              \
  ast_perror(ast, stderr, "\x1B[31;1mError:\x1B[0m");   \
  ast_destroy(ast);                                     \
  return 1;                                             \
}

/* Print the token for a node of the abstract syntax tree. */
void print_node(const ast_t *ast, const ast_node_t *node) {
  if (node->type == 1) {        /* AST_TOK_NUM */
    printf("\x1B[31;1m");
    switch (node->value.dtype) {
      case AST_DTYPE_BOOL:
        if (node->value.v.bval) printf("TRUE");
        else printf("FALSE");
        break;
      case AST_DTYPE_INT: printf("%d", node->value.v.ival); break;
      case AST_DTYPE_LONG: printf("%ld", node->value.v.lval); break;
      case AST_DTYPE_FLOAT: printf("%g", node->value.v.fval); break;
      case AST_DTYPE_DOUBLE: printf("%.8g", node->value.v.dval); break;
      default: printf("???"); break;
    }
    printf("\x1B[0m\n");
  }
  else if (node->type == 2) {   /* AST_TOK_STRING */
    printf("\x1B[35;1m%.*s\x1B[0m\n",
        (int) node->value.v.sval.len, node->value.v.sval.str);
  }
  else if (node->type == 3) {   /* AST_TOK_VAR */
    if (ast->vidx[node->value.v.lval] < 10)
      printf("\x1B[36;1m%c%ld\x1B[0m\n", AST_VAR_FLAG,
          ast->vidx[node->value.v.lval]);
    else
      printf("\x1B[36;1m%c%c%ld%c\x1B[0m\n", AST_VAR_FLAG, AST_VAR_START,
          node->value.v.lval, AST_VAR_END);
  }
  else if (node->type <= 30) {  /* operators */
    printf("\x1B[33;1m%s\x1B[0m\n", token[node->type]);
  }
  else {
    printf("???\n");
  }
}

/* Print the tree stucture, with at most 64 levels. */
void print_tree(const ast_t *ast, const ast_node_t *node, int level,
    uint64_t path) {
  if (!node || level >= 64) return;
  int i;
  uint64_t s = path;

  /* The bits of `path` are used as the indicator of the traversal path. */
  for (i = 0; i < level - 1; i++) {
    if (s & 1) printf("    ");          /* 1 for right child */
    else printf("|   ");                /* 0 for left child */
    s >>= 1;
  }
  if (i < level) {
    if (s & 1) printf("`-- ");
    else printf("|-- ");
  }
  print_node(ast, node);

  if (!node->right) print_tree(ast, node->left, level + 1, path | 1 << level);
  else {
    print_tree(ast, node->left, level + 1, path);
    print_tree(ast, node->right, level + 1, path | 1 << level);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s DTYPE EXPRESSION.\n\
Supported DTYPE: BOOL, INT, LONG, FLOAT, DOUBLE\n", argv[0]);
    return 1;
  }

  /* Determine the data type. */
  ast_dtype_t dtype;
  if (!strncmp(argv[1], "BOOL", 5)) dtype = AST_DTYPE_BOOL;
  else if (!strncmp(argv[1], "INT", 4)) dtype = AST_DTYPE_INT;
  else if (!strncmp(argv[1], "LONG", 5)) dtype = AST_DTYPE_LONG;
  else if (!strncmp(argv[1], "FLOAT", 6)) dtype = AST_DTYPE_FLOAT;
  else if (!strncmp(argv[1], "DOUBLE", 7)) dtype = AST_DTYPE_DOUBLE;
  else {
    fprintf(stderr, "Supported DTYPE: BOOL, INT, LONG, FLOAT, DOUBLE\n");
    return 1;
  }

  /* Initialise the interface. */
  ast_t *ast = ast_init();
  if (!ast) PRINT_ERROR(ast);

  /* Construct the tree. */
  if (ast_build(ast, argv[2], dtype, false)) PRINT_ERROR(ast);

  /* Print the tree. */
  ast_node_t *tree = (ast_node_t *) ast->ast;
  print_tree(ast, tree, 0, 0);

  /* Release memory. */
  ast_destroy(ast);
  return 0;
}
