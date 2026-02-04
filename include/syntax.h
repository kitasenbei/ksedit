#ifndef KSEDIT_SYNTAX_H
#define KSEDIT_SYNTAX_H

#include "types.h"

typedef enum {
    TOKEN_NORMAL,
    TOKEN_KEYWORD,
    TOKEN_TYPE,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_COMMENT,
    TOKEN_NUMBER,
    TOKEN_PREPROC,
    TOKEN_FUNCTION,
} TokenType;

typedef struct
{
    TokenType type;
    size_t    start;
    size_t    len;
} Token;

typedef struct
{
    Token* tokens;
    size_t count;
    size_t capacity;
    bool   in_multiline_comment;
} SyntaxState;

void      syntax_init(SyntaxState* state);
void      syntax_destroy(SyntaxState* state);
void      syntax_highlight_line(SyntaxState* state, const char* line, size_t len);
TokenType syntax_get_token_at(SyntaxState* state, size_t col);

#endif
