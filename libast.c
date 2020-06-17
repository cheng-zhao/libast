/*******************************************************************************
* libast.c: this file is part of the libast library.

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
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "libast.h"

/*============================================================================*\
                            Internal data structures
\*============================================================================*/

/* Definitions of tokens. */
typedef enum {
  AST_TOK_UNDEF,                /*  undefined token         */
  AST_TOK_PLUS,                 /*  `+` : plus              */
  AST_TOK_MINUS,                /*  `-` : minus             */
  AST_TOK_MUL,                  /*  `*` : multiply          */
  AST_TOK_DIV,                  /*  `/` : divide            */
  AST_TOK_POW,                  /*  `^` : power             */
  AST_TOK_NEG,                  /*  `-` : negative          */
  AST_TOK_AND,                  /* `&&` : and               */
  AST_TOK_OR,                   /* `||` : or                */
  AST_TOK_EQ,                   /* `==` : equal to          */
  AST_TOK_NEQ,                  /* `!=` : not equal to      */
  AST_TOK_GT,                   /*  `>` : greater than      */
  AST_TOK_GE,                   /* `>=` : greater or equal  */
  AST_TOK_LT,                   /*  `<` : less than         */
  AST_TOK_LE,                   /*  `<=`: less or equal     */
  AST_TOK_NOT,                  /*  `!` : not               */
  AST_TOK_PAREN_LEFT,           /*  `(` : left parenthesis  */
  AST_TOK_PAREN_RIGHT,          /*  `)` : right parenthesis */
  AST_TOK_SQRT,                 /* sqrt : square root       */
  AST_TOK_LN,                   /*   ln : log_e             */
  AST_TOK_LOG,                  /*  log : log_10            */
  AST_TOK_NUM,                  /*  a number                */
  AST_TOK_VAR                   /*  a variable              */
} ast_tok_t;

/* Types of the tokens. */
typedef enum {
  AST_TOKT_NULL,                /* undefined token          */
  AST_TOKT_UOPT,                /* a unary operator         */
  AST_TOKT_BOPT,                /* a binary operator        */
  AST_TOKT_PAREN,               /* parenthesis              */
  AST_TOKT_FUNC,                /* pre-defined function     */
  AST_TOKT_VALUE                /* number or variable       */
} ast_tok_type_t;

/* Operator attributes. */
typedef struct {
  ast_tok_type_t type;          /* type of the token        */
  int precedence;               /* precedence of the token  */
  int argc;                     /* number of arguments      */
  /* The associativity is always left-to-right. */
} ast_tok_attr_t;

static const ast_tok_attr_t ast_tok_attr[] = {
  /*    type    precedence  argc                               */
  {AST_TOKT_NULL,       -1,  1},        /* AST_TOK_UNDEF       */
  {AST_TOKT_BOPT,       4,   2},        /* AST_TOK_PLUS        */
  {AST_TOKT_BOPT,       4,   2},        /* AST_TOK_MINUS       */
  {AST_TOKT_BOPT,       5,   2},        /* AST_TOK_MUL         */
  {AST_TOKT_BOPT,       5,   2},        /* AST_TOK_DIV         */
  {AST_TOKT_BOPT,       6,   2},        /* AST_TOK_POW         */
  {AST_TOKT_UOPT,       8,   1},        /* AST_TOK_NEG         */
  {AST_TOKT_BOPT,       1,   2},        /* AST_TOK_AND         */
  {AST_TOKT_BOPT,       0,   2},        /* AST_TOK_OR          */
  {AST_TOKT_BOPT,       2,   2},        /* AST_TOK_EQ          */
  {AST_TOKT_BOPT,       2,   2},        /* AST_TOK_NEQ         */
  {AST_TOKT_BOPT,       3,   2},        /* AST_TOK_GT          */
  {AST_TOKT_BOPT,       3,   2},        /* AST_TOK_GE          */
  {AST_TOKT_BOPT,       3,   2},        /* AST_TOK_LT          */
  {AST_TOKT_BOPT,       3,   2},        /* AST_TOK_LE          */
  {AST_TOKT_UOPT,       7,   1},        /* AST_TOK_NOT         */
  {AST_TOKT_PAREN,      -1,  2},        /* AST_TOK_PAREN_LEFT  */
  {AST_TOKT_PAREN,      -1,  2},        /* AST_TOK_PAREN_RIGHT */
  {AST_TOKT_FUNC,       9,   1},        /* AST_TOK_SQRT        */
  {AST_TOKT_FUNC,       9,   1},        /* AST_TOK_LN          */
  {AST_TOKT_FUNC,       9,   1},        /* AST_TOK_LOG         */
  {AST_TOKT_VALUE,      99,  0},        /* AST_TOK_NUM         */
  {AST_TOKT_VALUE,      99,  0}         /* AST_TOK_VAR         */
};

/* Data type for interpreting numbers. */
typedef union {
  long l;
  double d;
} ast_value_t;

/* Data structure for storing error messages. */
typedef struct {
  int errno;                    /* identifier of the error */
  const char *msg;                    /* error message           */
  const char *str;
  const char *eptr;
} ast_error_t;

/* The abstract syntax tree (AST). */
typedef struct ast_tree_struct {
  ast_tok_t type;
  ast_value_t value;
  struct ast_tree_struct *parent;
  struct ast_tree_struct *left;
  struct ast_tree_struct *right;
} ast_node_t;


/*============================================================================*\
                           Definitions of error codes
\*============================================================================*/

#define AST_ERR_MEMORY          (-1)
#define AST_ERR_INIT            (-2)
#define AST_ERR_STRING          (-3)
#define AST_ERR_TOKEN           (-4)
#define AST_ERR_EXIST           (-5)
#define AST_ERR_NOEXP           (-6)
#define AST_ERR_VAR             (-7)
#define AST_ERR_VALUE           (-8)
#define AST_ERR_SIZE            (-9)
#define AST_ERR_EVAL            (-10)
#define AST_ERR_NVAR            (-11)
#define AST_ERR_UNKNOWN         (-99)

#define AST_ERRNO(ast)          (((ast_error_t *)ast->error)->errno)
#define AST_IS_ERROR(ast)       (AST_ERRNO(ast) != 0)


/*============================================================================*\
                       Functions for string manipulation
\*============================================================================*/

/******************************************************************************
Function `ast_skip_space`:
  Skip whitespaces.
Arguments:
  * `src`:      the input string.
Return:
  Pointer to the first non-whitespace character of the string.
******************************************************************************/
static inline const char *ast_skip_space(const char *src) {
  const char *dst = src;
  while (isspace(*dst)) dst++;
  return dst;
}

/******************************************************************************
Function `ast_msg`:
  Record the error message.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `msg`:      the null terminated error message;
  * `eptr`:     pointer to the character that triggers the error.
******************************************************************************/
static void ast_msg(ast_t *ast, const char *msg, const char *eptr) {
  if (!ast || !ast->error) return;
  ast_error_t *err = (ast_error_t *) ast->error;
  err->msg = msg;
  err->eptr = eptr;
}

/******************************************************************************
Function `ast_parse_num`:
  Convert a numerical token into a number.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `str`:      the input string;
  * `res`:      the resulting number;
  * `end`:      pointer to the first character that is not interpreted.
Return:
  Zero on success; non-zero on error.
******************************************************************************/
static int ast_parse_num(ast_t *ast, const char *str, ast_value_t *res,
    char **end) {
  /* Validate arguments. */
  if (!ast || AST_IS_ERROR(ast)) return AST_ERR_INIT;

  if (ast->dtype == AST_DTYPE_LONG) res->l = strtol(str, end, 10);
  else res->d = strtod(str, end);
  if (*end - str == 0) {                        /* no character is parsed */
    ast_msg(ast, "failed to recognise the number", str);
    return AST_ERRNO(ast) = AST_ERR_TOKEN;
  }
  return 0;
}


/*============================================================================*\
              Functions for the abstract syntax tree manipulation
\*============================================================================*/

/******************************************************************************
Function `ast_create`:
  Create a new node of the abstract syntax tree.
Arguments:
  * `type`:     type of the node;
  * `value`:    value of the node.
Return:
  The address of the node.
******************************************************************************/
static ast_node_t *ast_create(const ast_tok_t type, const ast_value_t value) {
  ast_node_t *node = malloc(sizeof *node);
  if (!node) return NULL;
  node->type = type;
  node->value = value;
  node->parent = node->left = node->right = NULL;
  return node;
}

/******************************************************************************
Function `ast_root`:
  Find the root node of the abstract syntax tree.
Arguments:
  * `node`:     an arbitrary node of the tree.
Return:
  Address of the root node.
******************************************************************************/
static inline ast_node_t *ast_root(ast_node_t *node) {
  while (node->parent) node = node->parent;
  return node;
}

/******************************************************************************
Function `ast_delete`:
  Deleta a node from the abstract syntax tree, once a pair of parenthesis
  is parsed.
Arguments:
  * `node`:     the node to be removed.
******************************************************************************/
static void ast_delete(ast_node_t *node) {
  /* This node should have only the left child. */
  ast_node_t *tmp = node->left;

  /* Copy contents from the left child. */
  node->type = tmp->type;
  node->value = tmp->value;
  node->left = tmp->left;
  node->right = tmp->right;

  /* Delete the left child. */
  free(tmp);
}

/******************************************************************************
Function `ast_insert`:
  Insert a node to the abstract syntax tree.
Arguments:
  * `node`:     current node of the tree;
  * `tok`:      token for the new node;
  * `value`:    value for the new node.
Return:
  The address of the inserted node.
******************************************************************************/
static ast_node_t *ast_insert(ast_node_t *node, const ast_tok_t tok,
    const ast_value_t value) {
  /* No need to ceate a new node if this is the first token. */
  if (node->type == AST_TOK_UNDEF) {
    node->type = tok;
    node->value = value;
    return node;
  }

  /* Create a new node. */
  ast_node_t *new = ast_create(tok, value);
  if (!new) return NULL;

  /* Insert a new node to the abstract syntax tree. */
  if (ast_tok_attr[tok].type == AST_TOKT_BOPT) {
    /* Find the right ancestor given the precedence. */
    while (node->parent && node->parent->type != AST_TOK_PAREN_LEFT &&
        ast_tok_attr[node->parent->type].type != AST_TOKT_FUNC &&
        ast_tok_attr[node->parent->type].precedence >=
        ast_tok_attr[tok].precedence) node = node->parent;
    new->left = node;
    if (node->parent) {                 /* insert between two nodes */
      new->parent = node->parent;
      if (node == node->parent->left) node->parent->left = new;
      else node->parent->right = new;
    }
    else node->parent = new;            /* new root */
    return new;
  }
  /* Insert an effective leaf to the abstract syntax tree. */
  else {        /* AST_TOK_PAREN_LEFT || AST_TOKT_UOPT/FUNC/VALUE */
    if (!node->left) {
      node->left = new;
      new->parent = node;
    }
    else {
      node->right = new;
      new->parent = node;
    }
  }
  return new;
}

/******************************************************************************
Function `ast_free`:
  Free memory allocated for the abstract syntax tree.
Arguments:
  * `node`:     the root node of the abstract syntax tree.
******************************************************************************/
static void ast_free(ast_node_t *node) {
  if (!node) return;
  ast_free(node->left);
  ast_free(node->right);
  free(node);
}


/*============================================================================*\
                  Functions for the initialisation and cleanup
\*============================================================================*/

/******************************************************************************
Function `ast_init`:
  Initialise the interface of the abstract syntax tree.
Return:
  The pointer to the interface on success; NULL on error.
******************************************************************************/
ast_t *ast_init(void) {
  ast_t *ast = malloc(sizeof *ast);
  if (!ast) return NULL;

  ast_error_t *err = malloc(sizeof(ast_error_t));
  if (!err) {
    free(ast);
    return NULL;
  }
  err->errno = 0;
  err->msg = err->str = err->eptr = NULL;
  ast->error = err;

  ast->nvar = 0;
  ast->ast = NULL;
  ast->vidx = NULL;
  return ast;
}

/******************************************************************************
Function `ast_destroy`:
  Release memory allocated for the abstract syntax tree.
Arguments:
  * `ast`:      interface of the abstract syntax tree.
******************************************************************************/
void ast_destroy(ast_t *ast) {
  if (!ast) return;
  free(ast->error);
  if (ast->vidx) free(ast->vidx);
  ast_free((ast_node_t *) ast->ast);
  free(ast);
}


/*============================================================================*\
                       Functions for recording variables
\*============================================================================*/

/******************************************************************************
Function `ast_bin_search`:
  Find the index of a value to be inserted to a sorted array.
Arguments:
  * `arr`:      pointer to the array;
  * `num`:      number of elements of the array;
  * `var`:      the value to be inserted.
Return:
  -1 if the array contains the value; otherwise the index for the insertion.
******************************************************************************/
static int ast_bin_search(const long *arr, const int num, const long var) {
  int i, l, u;
  l = 0;
  u = num - 1;
  while (l <= u) {
    i = (l + u) >> 1;
    if (arr[i] < var) l = i + 1;
    else if (arr[i] > var) u = i - 1;
    else return -1;     /* the element is already there */
  }
  return l;
}

/******************************************************************************
Function `ast_record_var`:
  Record the index of a variable if necessary;
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `idx`:      the index to be recorded;
******************************************************************************/
static void ast_record_var(ast_t *ast, const long idx) {
  const int pos = (ast->vidx) ? ast_bin_search(ast->vidx, ast->nvar, idx) : 0;
  if (pos < 0) return;          /* the index has already been recorded */

  if (ast->nvar == INT_MAX) {   /* there is no more space for the insertion */
    AST_ERRNO(ast) = AST_ERR_NVAR;
    return;
  }

  /* Check if the allocated space is enough. */
  if ((ast->nvar & (ast->nvar - 1)) == 0) {     /* nvar is 0 or power of 2 */
    int size = 1;
    if (INT_MAX / 2 < ast->nvar) size = INT_MAX;
    else if (ast->nvar) size = ast->nvar << 1;  /* double the size */

    long *tmp = realloc(ast->vidx, size * sizeof(long));
    if (!tmp) {
      AST_ERRNO(ast) = AST_ERR_MEMORY;
      return;
    }
    ast->vidx = tmp;
  }

  /* Right shift the existing elements. */
  if (ast->nvar && pos < ast->nvar)
    memmove(ast->vidx + pos + 1, ast->vidx + pos,
        (ast->nvar - pos) * sizeof(long));
  ast->vidx[pos] = idx;
  ast->nvar += 1;
}

/*============================================================================*\
                            Functions for the parser
\*============================================================================*/

/******************************************************************************
Function `ast_parse_token`:
  Parse the token and push it to the abstract syntax tree.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `src`:      the string on processing.
******************************************************************************/
static void ast_parse_token(ast_t *ast, ast_node_t *node, const char *src) {
  if (!ast || AST_IS_ERROR(ast)) return;
  const char *c = ast_skip_space(src);          /* skip whitespaces */

  /* Number of leaves. */
  int argc = 0;
  if (node->left) argc++;
  if (node->right) argc++;

  if (!(*c)) {
    /* Number of leaves is smaller than the arguments of this operator. */
    if (argc < ast_tok_attr[node->type].argc) {
      ast_msg(ast, "incomplete expression", src);
      AST_ERRNO(ast) = AST_ERR_TOKEN;
    }
    /* Open parenthesis. */
    do node = node->parent;
    while (node && node->type != AST_TOK_PAREN_LEFT &&
        ast_tok_attr[node->type].type != AST_TOKT_FUNC);
    if (node) {
      ast_msg(ast, "unclosed parenthesis", src);
      AST_ERRNO(ast) = AST_ERR_TOKEN;
    }
    return;
  }

  /* Check the token, so no check is performed in `ast_insert`. */
  ast_tok_t tok = AST_TOK_UNDEF;
  ast_value_t v = {0};
  switch (*c) {
    case '.':
    case 'i':                           /* for inf */
    case 'I':
    case 'n':                           /* for nan */
    case 'N':
      if (ast->dtype != AST_DTYPE_DOUBLE) break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      tok = AST_TOK_NUM;
      break;
    case AST_VAR_FLAG:                  /* deal with the variable */
      if (c[1] >= '1' && c[1] <= '9') {         /* number 1 - 9 */
        c++;
        v.l = *c - '1';         /* internal index starts from 0 */
        tok = AST_TOK_VAR;
      }
      else if (*(++c) == AST_VAR_START) {
        /* Get the variable index (long integer). */
        long idx = 0;
        for (++c; isdigit(*c); c++) {
          int digit = *c - '0';
          /* Overflow detection. */
          if (LONG_MAX / 10 < idx) {
            ast_msg(ast, "the variable index is too large", src);
            AST_ERRNO(ast) = AST_ERR_TOKEN;
            return;
          }
          idx *= 10;
          if (LONG_MAX - digit < idx) {
            ast_msg(ast, "the variable index is too large", src);
            AST_ERRNO(ast) = AST_ERR_TOKEN;
            return;
          }
          idx += digit;
        }
        if (idx > 0 && *c == AST_VAR_END) {
          v.l = idx - 1;        /* internal index starts from 0 */
          tok = AST_TOK_VAR;
        }
      }
      break;
    case '+': tok = AST_TOK_PLUS; break;
    case '-':
      if (argc >= ast_tok_attr[node->type].argc) tok = AST_TOK_MINUS;
      else tok = AST_TOK_NEG;
      break;
    case '*': tok = AST_TOK_MUL; break;
    case '/': tok = AST_TOK_DIV; break;
    case '^': tok = AST_TOK_POW; break;
    case '&':
      if (*(++c) == '&') tok = AST_TOK_AND;
      break;
    case '|':
      if (*(++c) == '|') tok = AST_TOK_OR;
      break;
    case '=':
      if (*(++c) == '=') tok = AST_TOK_OR;
      break;
    case '!':
      if (c[1] == '=') {
        c++;
        tok = AST_TOK_NEQ;
      }
      else tok = AST_TOK_NOT;
      break;
    case '>':
      if (c[1] == '=') {
        c++;
        tok = AST_TOK_GE;
      }
      else tok = AST_TOK_GT;
      break;
    case '<':
      if (c[1] == '=') {
        c++;
        tok = AST_TOK_LE;
      }
      else tok = AST_TOK_LT;
      break;
    case '(': tok = AST_TOK_PAREN_LEFT; break;
    case ')': tok = AST_TOK_PAREN_RIGHT; break;
    case 's':
      if (c[1] == 'q' && c[2] == 'r' && c[3] == 't' && c[4] == '(') {
        c += 4;
        tok = AST_TOK_SQRT;
      }
      break;
    case 'l':
      if (c[1] == 'n' && c[2] == '(') {
        c += 2;
        tok = AST_TOK_LN;
      }
      else if (c[1] == 'o' && c[2] == 'g' && c[3] == '(') {
        c += 3;
        tok = AST_TOK_LOG;
      }
    default: break;
  }

  if (tok == AST_TOK_UNDEF) ast_msg(ast, "unrecognised token", src);

  if(argc >= ast_tok_attr[node->type].argc) {
    /* This token can only be added as a new node. */
    if (tok == AST_TOK_PAREN_LEFT ||
        ast_tok_attr[tok].type == AST_TOKT_UOPT ||
        ast_tok_attr[tok].type == AST_TOKT_FUNC ||
        ast_tok_attr[tok].type == AST_TOKT_VALUE) {
      ast_msg(ast, "missing operator", src);
    }
  }
  else {
    /* This token can only be added as an effective leaf. */
    if (tok == AST_TOK_PAREN_RIGHT || ast_tok_attr[tok].type == AST_TOKT_BOPT)
      ast_msg(ast, "missing value", src);
  }

  /* Validate right parenthesis and remove the node if necessary. */
  if (tok == AST_TOK_PAREN_RIGHT) {
    if (node->type == AST_TOK_PAREN_LEFT)
      ast_msg(ast, "empty parenthesis", src);
    else {
      /* Check all the ancestors for left parenthesis or functions. */
      do node = node->parent;
      while (node && node->type != AST_TOK_PAREN_LEFT &&
          ast_tok_attr[node->type].type != AST_TOKT_FUNC);
      if (!node) ast_msg(ast, "unbalanced parenthesis", src);
      else if (node->type == AST_TOK_PAREN_LEFT) ast_delete(node);
    }
  }

  /* The error message is set. */
  if (((ast_error_t *) ast->error)->msg) {
    AST_ERRNO(ast) = AST_ERR_TOKEN;
    return;
  }

  /* Retrieve the value if this is a number. */
  if (tok == AST_TOK_NUM) {
    char *end;
    if (ast_parse_num(ast, c, &v, &end)) return;
    c = end;
  }
  else c++;

  /* Insert the token to the abstract syntax tree. */
  if (tok != AST_TOK_PAREN_RIGHT) {
    node = ast_insert(node, tok, v);
    if (!node) {
      AST_ERRNO(ast) = AST_ERR_MEMORY;
      return;
    }
  }

  /* Record the largest variable index. */
  if (tok == AST_TOK_VAR) ast_record_var(ast, v.l);

  /* Parse the next token. */
  ast_parse_token(ast, node, c);
}


/*============================================================================*\
                    Interfaces for the parser and evaluator
\*============================================================================*/

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
int ast_build(ast_t *ast, const char *str, const ast_dtype_t dtype) {
  if (!ast) return AST_ERR_INIT;
  if (ast->ast) return AST_ERRNO(ast) = AST_ERR_EXIST;
  ((ast_error_t *) ast->error)->str = str;

  if (!str || *(str = ast_skip_space(str)) == '\0')
    return AST_ERRNO(ast) = AST_ERR_STRING;

  ast->dtype = dtype;
  ast_value_t v = {0};
  ast->ast = ast_create(AST_TOK_UNDEF, v);
  if (!ast->ast) return AST_ERRNO(ast) = AST_ERR_MEMORY;

  ast_node_t *node = (ast_node_t *) ast->ast;
  ast_parse_token(ast, node, str);
  if (AST_IS_ERROR(ast)) return AST_ERRNO(ast);

  ast->ast = ast_root(node);
  return 0;
}

/******************************************************************************
Function `ast_eval_double`:
  Evaluate the value in double type, given the abstract syntax tree.
Arguments:
  * `node`:     a node of the abstract syntax tree;
  * `var`:      the variable array;
  * `err`:      an integer for the error status.
Return:
  The resulting double precision floating-point number.
******************************************************************************/
static double ast_eval_double(const ast_node_t* node, const double *var,
    int *err) {
  if (*err) return 0;
  if (node->type == AST_TOK_NUM) return node->value.d;
  else if (node->type == AST_TOK_VAR) return var[node->value.l];
  else if (ast_tok_attr[node->type].argc == 1) {
    double v = ast_eval_double(node->left, var, err);
    switch (node->type) {
      case AST_TOK_NEG: return -v;
      case AST_TOK_NOT:
        if (v != 0) return 0;
        else return 1;
      case AST_TOK_SQRT: return sqrt(v);
      case AST_TOK_LN: return log(v);
      case AST_TOK_LOG: return log10(v);
      default:
        *err = 1;
        return 0;
    }
  }
  else {
    double v1 = ast_eval_double(node->left, var, err);
    double v2 = ast_eval_double(node->right, var, err);
    switch (node->type) {
      case AST_TOK_PLUS: return v1 + v2;
      case AST_TOK_MINUS: return v1 - v2;
      case AST_TOK_MUL: return v1 * v2;
      case AST_TOK_DIV: return v1 / v2;
      case AST_TOK_POW: return pow(v1, v2);
      case AST_TOK_AND:
        if (v1 != 0 && v2 != 0) return 1;
        else return 0;
      case AST_TOK_OR:
        if (v1 != 0 || v2 != 0) return 1;
        else return 0;
      case AST_TOK_EQ:
        if (v1 == v2) return 1;
        else return 0;
      case AST_TOK_NEQ:
        if (v1 != v2) return 1;
        else return 0;
      case AST_TOK_GT:
        if (v1 > v2) return 1;
        else return 0;
      case AST_TOK_GE:
        if (v1 >= v2) return 1;
        else return 0;
      case AST_TOK_LT:
        if (v1 < v2) return 1;
        else return 0;
      case AST_TOK_LE:
        if (v1 <= v2) return 1;
        else return 0;
      default:
        *err = 1;
        return 0;
    }
  }
}

/******************************************************************************
Function `ast_eval_long`:
  Evaluate the value in long int type, given the abstract syntax tree.
Arguments:
  * `node`:     a node of the abstract syntax tree;
  * `var`:      the variable array;
  * `err`:      an integer for the error status.
Return:
  The resulting double precision floating-point number.
******************************************************************************/
static long ast_eval_long(const ast_node_t* node, const long *var, int *err) {
  if (*err) return 0;
  if (node->type == AST_TOK_NUM) return node->value.l;
  else if (node->type == AST_TOK_VAR) return var[node->value.l];
  else if (ast_tok_attr[node->type].argc == 1) {
    long v = ast_eval_long(node->left, var, err);
    switch (node->type) {
      case AST_TOK_NEG: return -v;
      case AST_TOK_NOT:
        if (v != 0) return 0;
        else return 1;
      case AST_TOK_SQRT: return sqrt(v);
      case AST_TOK_LN: return log(v);
      case AST_TOK_LOG: return log10(v);
      default:
        *err = 1;
        return 0;
    }
  }
  else {
    long v1 = ast_eval_long(node->left, var, err);
    long v2 = ast_eval_long(node->right, var, err);
    switch (node->type) {
      case AST_TOK_PLUS: return v1 + v2;
      case AST_TOK_MINUS: return v1 - v2;
      case AST_TOK_MUL: return v1 * v2;
      case AST_TOK_DIV: return v1 / v2;
      case AST_TOK_POW: return pow(v1, v2);
      case AST_TOK_AND:
        if (v1 != 0 && v2 != 0) return 1;
        else return 0;
      case AST_TOK_OR:
        if (v1 != 0 || v2 != 0) return 1;
        else return 0;
      case AST_TOK_EQ:
        if (v1 == v2) return 1;
        else return 0;
      case AST_TOK_NEQ:
        if (v1 != v2) return 1;
        else return 0;
      case AST_TOK_GT:
        if (v1 > v2) return 1;
        else return 0;
      case AST_TOK_GE:
        if (v1 >= v2) return 1;
        else return 0;
      case AST_TOK_LT:
        if (v1 < v2) return 1;
        else return 0;
      case AST_TOK_LE:
        if (v1 <= v2) return 1;
        else return 0;
      default:
        *err = 1;
        return 0;
    }
  }
}

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
int ast_eval(ast_t *ast, const void *var, const size_t size, void *value) {
  if (!ast) return AST_ERR_INIT;
  if (AST_IS_ERROR(ast)) return AST_ERRNO(ast);
  if (!ast->ast) return AST_ERRNO(ast) = AST_ERR_NOEXP;
  if (!var && size) return AST_ERRNO(ast) = AST_ERR_VAR;
  if (!value) return AST_ERRNO(ast) = AST_ERR_VALUE;
  if (ast->nvar && size <= ast->vidx[ast->nvar - 1])
    return AST_ERRNO(ast) = AST_ERR_SIZE;

  int err = 0;
  if (ast->dtype == AST_DTYPE_DOUBLE) {
    *((double *) value) =
      ast_eval_double((ast_node_t *) ast->ast, (double *) var, &err);
  }
  else {
    *((long *) value) =
      ast_eval_long((ast_node_t *) ast->ast, (long *) var, &err);
  }
  if (err) return AST_ERRNO(ast) = AST_ERR_EVAL;
  return 0;
}


/*============================================================================*\
                          Function for error handling
\*============================================================================*/

/******************************************************************************
Function `ast_perror`:
  Print the error message if there is an error.
Arguments:
  * `ast`:      interface of the abstract syntax tree;
  * `fp`:       output file stream;
  * `msg`:      string to be printed before the error message.
******************************************************************************/
void ast_perror(const ast_t *ast, FILE *fp, const char *msg) {
  const char *sep, *errmsg;
  if (!ast) {
    if (!msg || *msg == '\0') msg = sep = "";
    else sep = " ";
    fprintf(fp, "%s%sthe abstract syntax tree is not initialised.\n", msg, sep);
    return;
  }

  if(!(AST_IS_ERROR(ast))) return;
  const ast_error_t *err = (ast_error_t *) ast->error;
  switch (AST_ERRNO(ast)) {
    case AST_ERR_MEMORY:
      errmsg = "failed to allocate memory";
      break;
    case AST_ERR_INIT:
      errmsg = "the abstract syntax tree is not initialised";
      break;
    case AST_ERR_STRING:
      errmsg = "invalid expression string";
      break;
    case AST_ERR_TOKEN:
      if (err->msg) errmsg = err->msg;
      else errmsg = "uncaught error of the expression";
      break;
    case AST_ERR_EXIST:
      errmsg = "the abstract syntax tree has already been built";
      break;
    case AST_ERR_NOEXP:
      errmsg = "the abstract syntax tree has not been built";
      break;
    case AST_ERR_VAR:
      errmsg = "the variable array is not set";
      break;
    case AST_ERR_VALUE:
      errmsg = "value for the evaluation is not set";
      break;
    case AST_ERR_SIZE:
      errmsg = "not enough elements in the variable array";
      break;
    case AST_ERR_EVAL:
      errmsg = "unknown error for evaluation";
      break;
    case AST_ERR_NVAR:
      errmsg = "too many number of variables";
      break;
    default:
      errmsg = "unknown error";
      break;
  }

  if (!msg || *msg == '\0') msg = sep = "";
  else sep = " ";
  fprintf(fp, "%s%s%s.\n", msg, sep, errmsg);

  /* Print the specifier of the error location. */
  if (AST_ERRNO(ast) == AST_ERR_TOKEN && err->str && err->eptr) {
    fprintf(fp, "%s\n", err->str);
    for (size_t i = 0; i < err->eptr - err->str; i++) fprintf(fp, " ");
    fprintf(fp, "^\n");
  }
}

