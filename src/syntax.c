#include "syntax.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char* keywords[] = {
    "if", "else", "for", "while", "do", "switch", "case", "default",
    "break", "continue", "return", "goto", "sizeof", "typedef",
    "struct", "union", "enum", "static", "extern", "const", "volatile",
    "inline", "register", "auto", "restrict", "_Bool", "_Complex",
    "true", "false", "NULL", "nullptr",
    // C++ additions
    "class", "public", "private", "protected", "virtual", "override",
    "new", "delete", "this", "template", "typename", "namespace",
    "using", "try", "catch", "throw", "const_cast", "static_cast",
    "dynamic_cast", "reinterpret_cast", "explicit", "friend", "mutable",
    "operator", "constexpr", "noexcept", "final", "decltype", "auto",
    NULL
};

static const char* types[] = {
    "void", "char", "short", "int", "long", "float", "double",
    "signed", "unsigned", "bool", "size_t", "ssize_t", "ptrdiff_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
    "FILE", "string", "vector", "map", "set", "array",
    NULL
};

void syntax_init(SyntaxState* state)
{
    state->tokens               = malloc(sizeof(Token) * 256);
    state->count                = 0;
    state->capacity             = 256;
    state->in_multiline_comment = false;
}

void syntax_destroy(SyntaxState* state)
{
    free(state->tokens);
}

static void add_token(SyntaxState* state, TokenType type, size_t start, size_t len)
{
    if (state->count >= state->capacity) {
        state->capacity *= 2;
        state->tokens = realloc(state->tokens, sizeof(Token) * state->capacity);
    }
    state->tokens[state->count].type  = type;
    state->tokens[state->count].start = start;
    state->tokens[state->count].len   = len;
    state->count++;
}

static bool is_keyword(const char* word, size_t len)
{
    for (int i = 0; keywords[i]; i++) {
        if (strlen(keywords[i]) == len && strncmp(keywords[i], word, len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_type(const char* word, size_t len)
{
    for (int i = 0; types[i]; i++) {
        if (strlen(types[i]) == len && strncmp(types[i], word, len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_ident_char(char c)
{
    return isalnum(c) || c == '_';
}

void syntax_highlight_line(SyntaxState* state, const char* line, size_t len)
{
    state->count = 0;
    size_t i     = 0;

    // Continue multiline comment from previous line
    if (state->in_multiline_comment) {
        size_t start = 0;
        while (i < len - 1) {
            if (line[i] == '*' && line[i + 1] == '/') {
                add_token(state, TOKEN_COMMENT, start, i + 2 - start);
                state->in_multiline_comment = false;
                i += 2;
                break;
            }
            i++;
        }
        if (state->in_multiline_comment) {
            add_token(state, TOKEN_COMMENT, start, len - start);
            return;
        }
    }

    while (i < len) {
        char c = line[i];

        // Skip whitespace
        if (isspace(c)) {
            i++;
            continue;
        }

        // Preprocessor
        if (c == '#' && (i == 0 || !is_ident_char(line[i - 1]))) {
            add_token(state, TOKEN_PREPROC, i, len - i);
            return; // Rest of line is preprocessor
        }

        // Single-line comment
        if (c == '/' && i + 1 < len && line[i + 1] == '/') {
            add_token(state, TOKEN_COMMENT, i, len - i);
            return;
        }

        // Multi-line comment start
        if (c == '/' && i + 1 < len && line[i + 1] == '*') {
            size_t start = i;
            i += 2;
            while (i < len - 1) {
                if (line[i] == '*' && line[i + 1] == '/') {
                    add_token(state, TOKEN_COMMENT, start, i + 2 - start);
                    i += 2;
                    goto next;
                }
                i++;
            }
            // Comment continues to next line
            add_token(state, TOKEN_COMMENT, start, len - start);
            state->in_multiline_comment = true;
            return;
        }

        // String
        if (c == '"') {
            size_t start = i++;
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) {
                    i += 2;
                } else if (line[i] == '"') {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
            add_token(state, TOKEN_STRING, start, i - start);
            continue;
        }

        // Character
        if (c == '\'') {
            size_t start = i++;
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) {
                    i += 2;
                } else if (line[i] == '\'') {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
            add_token(state, TOKEN_CHAR, start, i - start);
            continue;
        }

        // Number
        if (isdigit(c) || (c == '.' && i + 1 < len && isdigit(line[i + 1]))) {
            size_t start = i;
            // Handle hex
            if (c == '0' && i + 1 < len && (line[i + 1] == 'x' || line[i + 1] == 'X')) {
                i += 2;
                while (i < len && (isxdigit(line[i]) || line[i] == '\''))
                    i++;
            } else {
                while (i < len && (isdigit(line[i]) || line[i] == '.' || line[i] == 'e' || line[i] == 'E' || line[i] == '\'' || line[i] == 'f' || line[i] == 'F' || line[i] == 'l' || line[i] == 'L' || line[i] == 'u' || line[i] == 'U'))
                    i++;
            }
            add_token(state, TOKEN_NUMBER, start, i - start);
            continue;
        }

        // Identifier/keyword/type
        if (isalpha(c) || c == '_') {
            size_t start = i;
            while (i < len && is_ident_char(line[i]))
                i++;
            size_t word_len = i - start;

            // Check if followed by ( -> function
            size_t j = i;
            while (j < len && isspace(line[j]))
                j++;

            if (is_keyword(line + start, word_len)) {
                add_token(state, TOKEN_KEYWORD, start, word_len);
            } else if (is_type(line + start, word_len)) {
                add_token(state, TOKEN_TYPE, start, word_len);
            } else if (j < len && line[j] == '(') {
                add_token(state, TOKEN_FUNCTION, start, word_len);
            } else {
                add_token(state, TOKEN_NORMAL, start, word_len);
            }
            continue;
        }

        i++;
    next:;
    }
}

TokenType syntax_get_token_at(SyntaxState* state, size_t col)
{
    for (size_t i = 0; i < state->count; i++) {
        Token* t = &state->tokens[i];
        if (col >= t->start && col < t->start + t->len) {
            return t->type;
        }
    }
    return TOKEN_NORMAL;
}
