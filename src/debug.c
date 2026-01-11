#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

/* Hexdump estilo */
void debug_print_hexdump(const unsigned char *data, int len) {
    printf("\n--- HEX DUMP ---\n");
    for (int i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("\n%04X: ", i);
        }
        printf("%02X ", data[i]);
    }
    printf("\n\n");
}

/* Converte TokenType para string descritiva */
const char* debug_token_name(TokenType type) {
    static char buf[32];
    
    switch (type) {
        case TOKEN_PLUS:         return "+";
        case TOKEN_MINUS:        return "-";
        case TOKEN_MULT:         return "*";
        case TOKEN_DIV:          return "/";
        case TOKEN_POW:          return "^";
        case TOKEN_LPAREN:       return "(";
        case TOKEN_RPAREN:       return ")";
        case TOKEN_NUMBER:       return "NUMBER";
        case TOKEN_VARIABLE_X:   return "x";
        case TOKEN_VARIABLE_THETA: return "theta";
        case TOKEN_VARIABLE_T:   return "t";
        case TOKEN_CONST_PI:     return "pi";
        case TOKEN_CONST_E:      return "e";
        case TOKEN_SIN:          return "sin";
        case TOKEN_COS:          return "cos";
        case TOKEN_TAN:          return "tan";
        case TOKEN_ABS:          return "abs";
        case TOKEN_SQRT:         return "sqrt";
        case TOKEN_END:          return "END";
        case TOKEN_ERROR:        return "ERROR";
        default:
            snprintf(buf, sizeof(buf), "UNKNOWN(%d)", type);
            return buf;
    }
}

/* Imprime tokens de forma legível */
void debug_print_tokens(const TokenBuffer *buf) {
    if (!buf || !buf->tokens) {
        printf("TokenBuffer vazia\n");
        return;
    }
    
    printf("\n--- TOKENS (%d) ---\n", buf->size);
    
    for (int i = 0; i < buf->size; i++) {
        Token *token = &buf->tokens[i];
        
        if (token->type == TOKEN_NUMBER) {
            printf("[%2d] %-12s value=%.6g\n", i, debug_token_name(token->type), token->value);
        } else if (token->type == TOKEN_END) {
            printf("[%2d] %-12s\n", i, debug_token_name(token->type));
        } else {
            printf("[%2d] %-12s (byte: %d)\n", i, debug_token_name(token->type), token->type);
        }
    }
    printf("\n");
}

/* Gera bytecode compactado (apenas os bytes de tipo, sem padding de struct) */
void debug_print_bytecode(const TokenBuffer *buf) {
    if (!buf || !buf->tokens) {
        printf("TokenBuffer vazia\n");
        return;
    }
    
    printf("\n--- BYTECODE COMPACTADO ---\n");
    
    /* Aloca buffer para bytecode */
    unsigned char *bytecode = malloc(buf->size);
    if (!bytecode) {
        printf("Erro ao alocar memória para bytecode\n");
        return;
    }
    
    /* Extrai apenas os bytes de TokenType (primeiro byte de cada Token) */
    for (int i = 0; i < buf->size; i++) {
        bytecode[i] = (unsigned char)(buf->tokens[i].type & 0xFF);
    }
    
    /* Imprime o bytecode com números de valor (para TOKEN_NUMBER) */
    printf("Sequência de bytes: ");
    for (int i = 0; i < buf->size; i++) {
        printf("%02X ", bytecode[i]);
    }
    printf("\n\nInterpretação:\n");
    
    for (int i = 0; i < buf->size; i++) {
        printf("  [%d] 0x%02X = %3d  ← %s", i, bytecode[i], bytecode[i], debug_token_name(buf->tokens[i].type));
        if (buf->tokens[i].type == TOKEN_NUMBER) {
            printf(" (valor: %.6g)", buf->tokens[i].value);
        }
        printf("\n");
    }
    
    /* Hexdump apenas do bytecode (16 bytes por linha) */
    printf("\nHex dump compactado:\n");
    debug_print_hexdump(bytecode, buf->size);
    
    free(bytecode);
}
