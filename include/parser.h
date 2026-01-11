#ifndef PARSER_H
#define PARSER_H

#include "tokens.h"

/* Define configuração global de locale */
extern LocaleConfig parser_locale;

/* Buffer para armazenar tokens */
typedef struct {
    Token *tokens;
    int size;
    int capacity;
} TokenBuffer;

/* Estados de parsing */
typedef enum {
    PARSER_OK = 0,
    PARSER_UNKNOWN_FUNCTION = 1,
    PARSER_UNKNOWN_VARIABLE = 2,
    PARSER_MIXED_VARIABLES = 3,
    PARSER_SYNTAX_ERROR = 4,
    PARSER_MEMORY_ERROR = 5
} ParserError;

/* Configura o locale para parsing */
void parser_set_locale(LocaleConfig locale);

/* Função principal de parsing */
ParserError parser_tokenize(const char *expr, TokenBuffer *output);

/* Função para converter para RPN */
ParserError parser_to_rpn(TokenBuffer *tokens, TokenBuffer *rpn);

/* Funções auxiliares */
void parser_init_buffer(TokenBuffer *buf);
void parser_free_buffer(TokenBuffer *buf);
int parser_add_token(TokenBuffer *buf, Token token);

#endif /* PARSER_H */
