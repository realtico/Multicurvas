#include <stdio.h>
#include <stdint.h>
#include "tokens.h"
#include "parser.h"

/* Programa para mostrar a economia de memória */

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║         ANÁLISE DE MEMÓRIA - Otimização Token            ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    /* Tamanhos das estruturas */
    printf("=== TAMANHOS DAS ESTRUTURAS ===\n\n");
    printf("TokenType (enum):        %2zu bytes\n", sizeof(TokenType));
    printf("uint16_t:                %2zu bytes\n", sizeof(uint16_t));
    printf("double:                  %2zu bytes\n", sizeof(double));
    printf("Token (otimizado):       %2zu bytes\n", sizeof(Token));
    printf("\n");
    
    /* Estrutura antiga (conceitual) */
    struct TokenOld {
        TokenType type;
        double value;
    };
    printf("Token (antigo, conceitual): %2zu bytes\n\n", sizeof(struct TokenOld));
    
    /* Economia */
    size_t old_size = sizeof(struct TokenOld);
    size_t new_size = sizeof(Token);
    size_t savings = old_size - new_size;
    double percent = 100.0 * savings / old_size;
    
    printf("=== ECONOMIA ===\n\n");
    printf("Redução por token:       %zu bytes (%.1f%%)\n", savings, percent);
    printf("\n");
    
    /* Exemplo prático */
    printf("=== EXEMPLO: Expressão 'sin(x) + 2 * 3.14' ===\n\n");
    
    parser_set_locale(LOCALE_POINT);
    TokenBuffer tokens;
    ParserError err = parser_tokenize("sin(x) + 2 * 3.14", &tokens);
    
    if (err == PARSER_OK) {
        int num_tokens = tokens.size;
        int num_values = tokens.values_size;
        
        printf("Número de tokens:        %d\n", num_tokens);
        printf("Números na expressão:    %d\n", num_values);
        printf("\n");
        
        size_t old_total = num_tokens * old_size;
        size_t new_tokens_size = num_tokens * new_size;
        size_t new_values_size = num_values * sizeof(double);
        size_t new_total = new_tokens_size + new_values_size;
        
        printf("Memória (estrutura antiga):  %zu bytes\n", old_total);
        printf("Memória (otimizada):\n");
        printf("  - Array de tokens:         %zu bytes\n", new_tokens_size);
        printf("  - Array de valores:        %zu bytes\n", new_values_size);
        printf("  - Total:                   %zu bytes\n", new_total);
        printf("\n");
        printf("Economia total:              %zu bytes (%.1f%%)\n", 
               old_total - new_total, 
               100.0 * (old_total - new_total) / old_total);
        printf("\n");
        
        /* Cache locality */
        printf("=== BENEFÍCIOS DE CACHE ===\n\n");
        printf("Tokens por cache line (64 bytes):\n");
        printf("  - Estrutura antiga:      %zu tokens\n", 64 / old_size);
        printf("  - Estrutura otimizada:   %zu tokens\n", 64 / new_size);
        printf("\n");
        printf("Melhoria: %.1fx mais tokens por cache line!\n", 
               (double)(64 / new_size) / (64 / old_size));
        
        parser_free_buffer(&tokens);
    }
    
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                    Análise Completa                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
