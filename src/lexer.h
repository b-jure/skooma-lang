#ifndef SKOOMA_LEXER_H
#define SKOOMA_LEXER_H

#include "common.h"
#include "mem.h"
#include "value.h"

#include <stdarg.h>

typedef enum {
    // Single character tokens.
    TOK_LBRACK = 0,
    TOK_RBRACK,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_DOT,
    TOK_DOT_DOT_DOT,
    TOK_COMMA,
    TOK_MINUS,
    TOK_PLUS,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_SLASH,
    TOK_STAR,
    TOK_PERCENT,
    TOK_CARET,
    TOK_QMARK,
    // One or two character tokens.
    TOK_BANG,
    TOK_BANG_EQUAL,
    TOK_EQUAL,
    TOK_EQUAL_EQUAL,
    TOK_GREATER,
    TOK_GREATER_EQUAL,
    TOK_LESS,
    TOK_LESS_EQUAL,
    // Literals.
    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_NUMBER,
    // Keywords.
    TOK_AND,
    TOK_BREAK,
    TOK_CASE,
    TOK_CONTINUE,
    TOK_CLASS,
    TOK_DEFAULT,
    TOK_ELSE,
    TOK_FALSE,
    TOK_FOR,
    TOK_FOREACH,
    TOK_FN,
    TOK_IF,
    TOK_IN,
    TOK_IMPL,
    TOK_NIL,
    TOK_OR,
    TOK_RETURN,
    TOK_SUPER,
    TOK_SELF,
    TOK_SWITCH,
    TOK_TRUE,
    TOK_VAR,
    TOK_WHILE,
    TOK_LOOP,
    TOK_FIXED,

    TOK_ERROR,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType   type;
    const char* start; // slice start
    UInt        len; // slice length
    UInt        line; // source file line
    Value       value; // constant value
} Token;

typedef struct {
    VM*         vm; // virtual machine
    const char* source; // source file
    const char* start; // slice/token start
    const char* _current; // current byte in the source file
    Token       previous;
    Token       current;
    UInt        line; // source file line
    bool        panic; // sync flag
    bool        error; // parse error flag
} Lexer; // Lexer



Lexer     L_new(const char* source, VM* vm);
Token scan(Lexer* lexer);
Token syntoken(const char* name);
void  printerror(Lexer* parser, const char* fmt, va_list args);

#endif
