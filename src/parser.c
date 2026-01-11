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
static int is_operator(TokenType type);
static int is_function(TokenType type);
static int is_variable(TokenType type);
static int is_constant(TokenType type);

/* Configura o locale para parsing */
void parser_set_locale(LocaleConfig locale) {
    parser_locale = locale;
}

/* Inicializa buffer */
void parser_init_buffer(TokenBuffer *buf) {
    buf->capacity = 64;
    buf->size = 0;
    buf->tokens = malloc(buf->capacity * sizeof(Token));
    
    buf->values_capacity = 16;  /* Menos valores que tokens */
    buf->values_size = 0;
    buf->values = malloc(buf->values_capacity * sizeof(double));
}

/* Libera buffer */
void parser_free_buffer(TokenBuffer *buf) {
    free(buf->tokens);
    buf->tokens = NULL;
    buf->size = 0;
    buf->capacity = 0;
    
    free(buf->values);
    buf->values = NULL;
    buf->values_size = 0;
    buf->values_capacity = 0;
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

/* Adiciona valor numérico ao buffer e retorna o índice */
static int parser_add_value(TokenBuffer *buf, double value) {
    if (buf->values_size >= buf->values_capacity) {
        buf->values_capacity *= 2;
        double *new_values = realloc(buf->values, buf->values_capacity * sizeof(double));
        if (!new_values) return -1;
        buf->values = new_values;
    }
    buf->values[buf->values_size] = value;
    return buf->values_size++;
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
    CHECK_KEYWORD("exp", TOKEN_EXP);
    CHECK_KEYWORD("log", TOKEN_LOG);
    CHECK_KEYWORD("log10", TOKEN_LOG10);
    CHECK_KEYWORD("sinh", TOKEN_SINH);
    CHECK_KEYWORD("cosh", TOKEN_COSH);
    CHECK_KEYWORD("tanh", TOKEN_TANH);
    CHECK_KEYWORD("asin", TOKEN_ASIN);
    CHECK_KEYWORD("acos", TOKEN_ACOS);
    CHECK_KEYWORD("atan", TOKEN_ATAN);
    CHECK_KEYWORD("asinh", TOKEN_ASINH);
    CHECK_KEYWORD("acosh", TOKEN_ACOSH);
    CHECK_KEYWORD("atanh", TOKEN_ATANH);
    CHECK_KEYWORD("ceil", TOKEN_CEIL);
    CHECK_KEYWORD("floor", TOKEN_FLOOR);
    CHECK_KEYWORD("frac", TOKEN_FRAC);
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
                int value_idx = parser_add_value(output, value);
                if (value_idx < 0) {
                    parser_free_buffer(output);
                    return PARSER_MEMORY_ERROR;
                }
                token.type = TOKEN_NUMBER;
                token.value_index = (uint16_t)value_idx;
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
            token.value_index = 0;
            if (!parser_add_token(output, token)) {
                parser_free_buffer(output);
                return PARSER_MEMORY_ERROR;
            }
            continue;
        }
        
        /* Caracteres especiais e operadores */
        switch (expr[i]) {
            case '+': case '*': case '/': case '^':
            case '(': case ')':
                token.type = (TokenType)expr[i];
                token.value_index = 0;
                if (!parser_add_token(output, token)) {
                    parser_free_buffer(output);
                    return PARSER_MEMORY_ERROR;
                }
                i++;
                break;
            
            case '-':
                /* Detecta se é operador unário (negativo) */
                /* É unário se: início da expressão, depois de '(', ou depois de operador */
                int is_unary = 0;
                if (output->size == 0) {
                    /* Início da expressão */
                    is_unary = 1;
                } else {
                    /* Verifica token anterior */
                    TokenType prev = output->tokens[output->size - 1].type;
                    if (prev == TOKEN_LPAREN || prev == TOKEN_PLUS || 
                        prev == TOKEN_MINUS || prev == TOKEN_MULT || 
                        prev == TOKEN_DIV || prev == TOKEN_POW) {
                        is_unary = 1;
                    }
                }
                
                if (is_unary) {
                    /* Insere 0 antes do menos para transformar em "0 - x" */
                    int zero_idx = parser_add_value(output, 0.0);
                    if (zero_idx < 0) {
                        parser_free_buffer(output);
                        return PARSER_MEMORY_ERROR;
                    }
                    Token zero_token = {TOKEN_NUMBER, (uint16_t)zero_idx};
                    if (!parser_add_token(output, zero_token)) {
                        parser_free_buffer(output);
                        return PARSER_MEMORY_ERROR;
                    }
                }
                
                /* Adiciona o operador menos */
                token.type = TOKEN_MINUS;
                token.value_index = 0;
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
        
        if (is_variable(type)) {
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

/* Retorna precedência de um operador (maior = mais prioritário) */
static int get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_POW:
            return 4;
        case TOKEN_MULT:
        case TOKEN_DIV:
            return 3;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 2;
        default:
            /* Funções retornam 0, mas esse valor nunca é consultado.
               Na prática, funções têm precedência "infinita" estrutural:
               bloqueiam desempilhamento via is_operator() e só saem no ')' */
            return 0;
    }
}

/* Verifica se o token é um operador */
static int is_operator(TokenType type) {
    return (type == TOKEN_PLUS || type == TOKEN_MINUS || 
            type == TOKEN_MULT || type == TOKEN_DIV || type == TOKEN_POW);
}

/* Verifica se o token é uma função (usa range para extensibilidade) */
static int is_function(TokenType type) {
    return (type >= TOKEN_FUNCTION_START && type <= TOKEN_FUNCTION_END);
}

/* Verifica se o token é uma variável (usa range para extensibilidade) */
static int is_variable(TokenType type) {
    return (type >= TOKEN_VARIABLE_START && type <= TOKEN_VARIABLE_END);
}

/* Verifica se o token é uma constante (usa range para extensibilidade) */
static int is_constant(TokenType type) {
    return (type >= TOKEN_CONST_START && type <= TOKEN_CONST_END);
}

/* Algoritmo Shunting Yard - Converte infixa para RPN */
ParserError parser_to_rpn(TokenBuffer *tokens, TokenBuffer *rpn) {
    if (!tokens || !rpn) return PARSER_SYNTAX_ERROR;
    
    parser_init_buffer(rpn);
    
    /* Copia o array de valores (compartilhado entre tokens e rpn) */
    if (tokens->values_size > 0) {
        rpn->values = malloc(tokens->values_size * sizeof(double));
        if (!rpn->values) {
            parser_free_buffer(rpn);
            return PARSER_MEMORY_ERROR;
        }
        memcpy(rpn->values, tokens->values, tokens->values_size * sizeof(double));
        rpn->values_size = tokens->values_size;
        rpn->values_capacity = tokens->values_size;
    }
    
    /* Pilha de operadores */
    Token *stack = malloc(tokens->size * sizeof(Token));
    if (!stack) return PARSER_MEMORY_ERROR;
    int stack_top = -1;
    
    /* Processa cada token */
    for (int i = 0; i < tokens->size; i++) {
        Token token = tokens->tokens[i];
        TokenType type = token.type;
        
        /* Fim da expressão - para de processar */
        if (type == TOKEN_END) break;
        
        /* Números, variáveis e constantes vão direto para a saída */
        if (type == TOKEN_NUMBER || is_variable(type) || is_constant(type)) {
            if (!parser_add_token(rpn, token)) {
                free(stack);
                parser_free_buffer(rpn);
                return PARSER_MEMORY_ERROR;
            }
        }
        /* Funções vão para a pilha */
        else if (is_function(type)) {
            stack[++stack_top] = token;
        }
        /* Parêntese esquerdo vai para a pilha */
        else if (type == TOKEN_LPAREN) {
            stack[++stack_top] = token;
        }
        /* Parêntese direito: desempilha até encontrar '(' */
        else if (type == TOKEN_RPAREN) {
            while (stack_top >= 0 && stack[stack_top].type != TOKEN_LPAREN) {
                if (!parser_add_token(rpn, stack[stack_top--])) {
                    free(stack);
                    parser_free_buffer(rpn);
                    return PARSER_MEMORY_ERROR;
                }
            }
            
            /* Remove o '(' da pilha */
            if (stack_top >= 0) stack_top--;
            
            /* Se há uma função no topo, desempilha ela também */
            if (stack_top >= 0 && is_function(stack[stack_top].type)) {
                if (!parser_add_token(rpn, stack[stack_top--])) {
                    free(stack);
                    parser_free_buffer(rpn);
                    return PARSER_MEMORY_ERROR;
                }
            }
        }
        /* Operadores */
        else if (is_operator(type)) {
            /* Desempilha operadores de maior ou igual precedência */
            /* Exceto para ^, que é associativo à direita */
            int prec = get_precedence(type);
            
            while (stack_top >= 0 && is_operator(stack[stack_top].type)) {
                int stack_prec = get_precedence(stack[stack_top].type);
                
                /* Para ^: apenas desempilha se precedência for MAIOR (associativo à direita) */
                /* Para outros: desempilha se precedência for MAIOR OU IGUAL */
                if (type == TOKEN_POW) {
                    if (stack_prec <= prec) break;
                } else {
                    if (stack_prec < prec) break;
                }
                
                if (!parser_add_token(rpn, stack[stack_top--])) {
                    free(stack);
                    parser_free_buffer(rpn);
                    return PARSER_MEMORY_ERROR;
                }
            }
            
            stack[++stack_top] = token;
        }
    }
    
    /* Desempilha todos os operadores restantes */
    while (stack_top >= 0) {
        if (!parser_add_token(rpn, stack[stack_top--])) {
            free(stack);
            parser_free_buffer(rpn);
            return PARSER_MEMORY_ERROR;
        }
    }
    
    /* Adiciona token de fim */
    Token end_token = {TOKEN_END, 0};
    if (!parser_add_token(rpn, end_token)) {
        free(stack);
        parser_free_buffer(rpn);
        return PARSER_MEMORY_ERROR;
    }
    
    free(stack);
    return PARSER_OK;
}
