#ifndef DEBUG_H
#define DEBUG_H

#include "tokens.h"
#include "parser.h"

/* Imprime um hexdump da buffer de tokens */
void debug_print_hexdump(const unsigned char *data, int len);

/* Imprime tokens de forma legível */
void debug_print_tokens(const TokenBuffer *buf);

/* Gera bytecode compactado e imprime hexdump dos tokens (sem overhead de struct) */
void debug_print_bytecode(const TokenBuffer *buf);

/* Converte um token para string descritiva */
const char* debug_token_name(TokenType type);

#endif /* DEBUG_H */
