#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "parser.h"

/* Configuração global */
LocaleConfig parser_locale = LOCALE_POINT;  /* Padrão: ponto decimal */

/* Forward declarations */
static ParserError parser_check_syntax(TokenBuffer *tokens);
static ParserError parser_check_variables(TokenBuffer *tokens);

/* Configura o locale para parsing */
void parser_set_locale(LocaleConfig locale) {
    parser_locale = locale;
}

/* Inicializa buffer */
void parser_init_buffer(TokenBuffer *buf) {
    buf->capacity = 64;
    buf->size = 0;
    buf->tokens = malloc(buf->capacity * sizeof(Token));
}

/* Libera buffer */
void parser_free_buffer(TokenBuffer *buf) {
    free(buf->tokens);
    buf->tokens = NULL;
    buf->size = 0;
    buf->capacity = 0;
}

/* Adiciona token ao buffer (com realocação se necessário) */
int parser_add_token(TokenBuffer *buf, Token token) {
    if (buf->size >= buf->capacity) {
        buf->capacity *= 2;
        Token *new_tokens = realloc(buf->tokens, buf->capacity * sizeof(Token));
        if (!new_tokens) return 0;
        buf->tokens = new_tokens;
    }
    buf->tokens[buf->size++] = token;
    return 1;
}

/* Tenta fazer parse de um número */
static int try_parse_number(const char *str, int *pos, double *value) {
    char buffer[64];  /* Buffer para normalizar marca decimal */
    const char *src = str + *pos;
    char *dst = buffer;
    char dec_mark = (parser_locale == LOCALE_COMMA) ? ',' : '.';
    int has_decimal = 0;
    int digit_count = 0;
    
    /* Copia dígitos normalizando marca decimal para ponto (padrão C) */
    while (dst - buffer < (int)sizeof(buffer) - 2 && 
           (isdigit(*src) || (*src == dec_mark && !has_decimal))) {
        
        if (*src == dec_mark) {
            *dst++ = '.';  /* Normaliza para ponto */
            has_decimal = 1;
            src++;
        } else {
            *dst++ = *src++;
            digit_count++;
        }
    }
    *dst = '\0';
    
    /* Precisa ter pelo menos um dígito */
    if (digit_count == 0) {
        return 0;
    }
    
    char *endptr;
    double num = strtod(buffer, &endptr);
    
    if (endptr == buffer) {
        return 0;
    }
    
    *value = num;
    *pos += (endptr - buffer);
    
    return 1;
}

/* Tenta fazer parse de uma palavra-chave (função, constante, variável) */
static TokenType try_parse_keyword(const char *str, int *pos) {
    #define CHECK_KEYWORD(keyword, token) \
        if (strncmp(str + *pos, keyword, strlen(keyword)) == 0 && \
            !isalnum(str[*pos + strlen(keyword)])) { \
            *pos += strlen(keyword); \
            return token; \
        }
    
    CHECK_KEYWORD("sin", TOKEN_SIN);
    CHECK_KEYWORD("cos", TOKEN_COS);
    CHECK_KEYWORD("tan", TOKEN_TAN);
    CHECK_KEYWORD("abs", TOKEN_ABS);
    CHECK_KEYWORD("sqrt", TOKEN_SQRT);
    CHECK_KEYWORD("pi", TOKEN_CONST_PI);
    CHECK_KEYWORD("e", TOKEN_CONST_E);
    CHECK_KEYWORD("theta", TOKEN_VARIABLE_THETA);
    CHECK_KEYWORD("x", TOKEN_VARIABLE_X);
    CHECK_KEYWORD("t", TOKEN_VARIABLE_T);
    
    #undef CHECK_KEYWORD
    
    return TOKEN_ERROR;  /* Palavra-chave desconhecida */
}

/* Tokenizador principal */
ParserError parser_tokenize(const char *expr, TokenBuffer *output) {
    if (!expr || !output) return PARSER_SYNTAX_ERROR;
    
    parser_init_buffer(output);
    
    int i = 0;
    while (expr[i] != '\0') {
        /* Pula espaços em branco */
        if (isspace(expr[i])) {
            i++;
            continue;
        }
        
        Token token = {0};
        
        /* Tenta fazer parse de número */
        char dec_mark = (parser_locale == LOCALE_COMMA) ? ',' : '.';
        if (isdigit(expr[i]) || (expr[i] == dec_mark && isdigit(expr[i+1]))) {
            double value;
            if (try_parse_number(expr, &i, &value)) {
                token.type = TOKEN_NUMBER;
                token.value = value;
                if (!parser_add_token(output, token)) {
                    parser_free_buffer(output);
                    return PARSER_MEMORY_ERROR;
                }
                continue;
            }
        }
        
        /* Tenta fazer parse de palavra-chave */
        if (isalpha(expr[i])) {
            TokenType kw_type = try_parse_keyword(expr, &i);
            
            if (kw_type == TOKEN_ERROR) {
                /* Palavra-chave desconhecida */
                parser_free_buffer(output);
                return PARSER_UNKNOWN_FUNCTION;
            }
            
            token.type = kw_type;
            token.value = 0;
            if (!parser_add_token(output, token)) {
                parser_free_buffer(output);
                return PARSER_MEMORY_ERROR;
            }
            continue;
        }
        
        /* Caracteres especiais e operadores */
        switch (expr[i]) {
            case '+': case '-': case '*': case '/': case '^':
            case '(': case ')':
                token.type = (TokenType)expr[i];
                token.value = 0;
                if (!parser_add_token(output, token)) {
                    parser_free_buffer(output);
                    return PARSER_MEMORY_ERROR;
                }
                i++;
                break;
            
            default:
                /* Caractere inválido */
                parser_free_buffer(output);
                return PARSER_SYNTAX_ERROR;
        }
    }
    
    /* Adiciona token de fim */
    Token end_token = {TOKEN_END, 0};
    if (!parser_add_token(output, end_token)) {
        parser_free_buffer(output);
        return PARSER_MEMORY_ERROR;
    }
    
    /* Valida */
    ParserError err = parser_check_variables(output);
    if (err != PARSER_OK) {
        parser_free_buffer(output);
        return err;
    }
    
    err = parser_check_syntax(output);
    if (err != PARSER_OK) {
        parser_free_buffer(output);
        return err;
    }
    
    return PARSER_OK;
}

/* Verifica se as variáveis são consistentes (não mistura x, theta, t) */
static ParserError parser_check_variables(TokenBuffer *tokens) {
    TokenType found_var = TOKEN_END;
    
    for (int i = 0; i < tokens->size; i++) {
        TokenType type = tokens->tokens[i].type;
        
        if (type == TOKEN_VARIABLE_X || type == TOKEN_VARIABLE_THETA || type == TOKEN_VARIABLE_T) {
            if (found_var == TOKEN_END) {
                found_var = type;
            } else if (found_var != type) {
                return PARSER_MIXED_VARIABLES;
            }
        }
    }
    
    return PARSER_OK;
}

/* Verifica sintaxe básica da expressão */
static ParserError parser_check_syntax(TokenBuffer *tokens) {
    int paren_depth = 0;
    
    for (int i = 0; i < tokens->size - 1; i++) {  /* -1 para pular TOKEN_END */
        TokenType curr = tokens->tokens[i].type;
        
        /* Verifica parênteses balanceados */
        if (curr == TOKEN_LPAREN) paren_depth++;
        else if (curr == TOKEN_RPAREN) {
            paren_depth--;
            if (paren_depth < 0) return PARSER_SYNTAX_ERROR;
        }
    }
    
    if (paren_depth != 0) return PARSER_SYNTAX_ERROR;
    
    return PARSER_OK;
}

/* Stub: será implementado depois */
ParserError parser_to_rpn(TokenBuffer *tokens, TokenBuffer *rpn) {
    /* Por enquanto, apenas copia os tokens */
    parser_init_buffer(rpn);
    
    for (int i = 0; i < tokens->size; i++) {
        if (!parser_add_token(rpn, tokens->tokens[i])) {
            parser_free_buffer(rpn);
            return PARSER_MEMORY_ERROR;
        }
    }
    
    return PARSER_OK;
}
