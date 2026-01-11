#ifndef TOKENS_H
#define TOKENS_H

#include <stdint.h>

/* Configuração de locale - marca decimal */
typedef enum {
    LOCALE_POINT,      /* 3.14 (padrão C/EN) */
    LOCALE_COMMA       /* 3,14 (PT-BR, FR, DE) */
} LocaleConfig;

/* Token types - valores >= 128 são especiais (funções, constantes, variáveis) */
typedef enum {
    /* Operadores (ASCII) */
    TOKEN_PLUS       = '+',
    TOKEN_MINUS      = '-',
    TOKEN_MULT       = '*',
    TOKEN_DIV        = '/',
    TOKEN_POW        = '^',
    TOKEN_LPAREN     = '(',
    TOKEN_RPAREN     = ')',
    
    /* Especiais >= 128 */
    TOKEN_NUMBER     = 128,
    TOKEN_VARIABLE_X = 129,
    TOKEN_VARIABLE_THETA = 130,
    TOKEN_VARIABLE_T = 131,
    
    TOKEN_CONST_PI   = 140,
    TOKEN_CONST_E    = 141,
    
    TOKEN_SIN        = 150,
    TOKEN_COS        = 151,
    TOKEN_TAN        = 152,
    TOKEN_ABS        = 153,
    TOKEN_SQRT       = 154,
    
    TOKEN_END        = 255,  /* Fim da expressão */
    TOKEN_ERROR      = 256   /* Erro de parsing */
} TokenType;

typedef struct {
    TokenType type;
    double value;      /* Para TOKEN_NUMBER */
} Token;

#endif /* TOKENS_H */
