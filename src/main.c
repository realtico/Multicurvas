#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "debug.h"

/* Função helper para testar uma expressão */
static void test_expression(const char *expr) {
    printf("\n========================================\n");
    printf("Testando: \"%s\"\n", expr);
    printf("========================================\n");
    
    TokenBuffer tokens;
    TokenBuffer rpn;
    
    /* Tokeniza */
    ParserError err = parser_tokenize(expr, &tokens);
    
    if (err != PARSER_OK) {
        printf("ERRO: ");
        switch (err) {
            case PARSER_UNKNOWN_FUNCTION:
                printf("Função desconhecida\n");
                break;
            case PARSER_UNKNOWN_VARIABLE:
                printf("Variável desconhecida\n");
                break;
            case PARSER_MIXED_VARIABLES:
                printf("Variáveis misturadas (não use x, theta, t juntos)\n");
                break;
            case PARSER_SYNTAX_ERROR:
                printf("Erro de sintaxe\n");
                break;
            case PARSER_MEMORY_ERROR:
                printf("Erro de memória\n");
                break;
            default:
                printf("Erro desconhecido (%d)\n", err);
        }
        return;
    }
    
    printf("✓ Tokenização OK\n");
    debug_print_tokens(&tokens);
    
    /* Hexdump dos tokens */
    printf("Hexdump dos tokens:\n");
    debug_print_hexdump((const unsigned char *)tokens.tokens, tokens.size * sizeof(Token));
    
    /* Converte para RPN (stub por enquanto) */
    err = parser_to_rpn(&tokens, &rpn);
    if (err == PARSER_OK) {
        printf("✓ Conversão para RPN OK (ainda é cópia - será implementado depois)\n");
        debug_print_tokens(&rpn);
        parser_free_buffer(&rpn);
    }
    
    parser_free_buffer(&tokens);
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║    MULTICURVAS - Parser de Expressões Matemáticas        ║\n");
    printf("║              Fase 1: Tokenizador + RPN                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    /* TESTE 1: Com locale POINT (padrão) */
    printf("\n▶▶▶ TESTE COM LOCALE POINT (ponto decimal) ▶▶▶\n");
    parser_set_locale(LOCALE_POINT);
    
    test_expression("sin(x)*2+x");
    test_expression("9*(theta-pi/2)");
    test_expression("2*e^(-t/2)");
    test_expression("3.14159");
    
    /* TESTE 2: Com locale COMMA */
    printf("\n\n▶▶▶ TESTE COM LOCALE COMMA (vírgula decimal) ▶▶▶\n");
    parser_set_locale(LOCALE_COMMA);
    
    test_expression("sin(x)*2+x");
    test_expression("9*(theta-pi/2)");
    test_expression("2*e^(-t/2)");
    test_expression("3,14159");
    
    /* TESTE 3: Erros diversos */
    printf("\n\n▶▶▶ TESTES COM ERROS ▶▶▶\n");
    parser_set_locale(LOCALE_POINT);
    
    test_expression("cossecante(x)");           /* Função desconhecida */
    test_expression("x + theta");               /* Variáveis misturadas */
    test_expression("sin(x))");                 /* Parênteses desbalanceados */
    test_expression("pi + e");                  /* Constantes */
    
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                    Testes Completos                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
