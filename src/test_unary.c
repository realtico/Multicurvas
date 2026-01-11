#include <stdio.h>
#include "parser.h"
#include "evaluator.h"
#include "debug.h"

int main(void) {
    parser_set_locale(LOCALE_POINT);
    
    const char *tests[] = {
        "2*e^(-x/2)",
        "-x",
        "-x+3",
        "2*(-x)",
        "sin(-x)",
        "-2*x",
        "x+-3",
        "(-x)^2"
    };
    
    for (int t = 0; t < 8; t++) {
        const char *expr = tests[t];
        printf("\n========================================\n");
        printf("Testando: %s\n", expr);
        printf("========================================\n");
        
        TokenBuffer tokens;
        ParserError err = parser_tokenize(expr, &tokens);
        
        if (err != PARSER_OK) {
            printf("✗ Erro no parsing: %d\n", err);
            continue;
        }
        
        TokenBuffer rpn;
        err = parser_to_rpn(&tokens, &rpn);
        
        if (err != PARSER_OK) {
            printf("✗ Erro na conversão RPN: %d\n", err);
            parser_free_buffer(&tokens);
            continue;
        }
        
        printf("✓ Parse OK - Testando avaliação:\n");
        EvalResult result = evaluator_eval_rpn(&rpn, 2.0);
        if (result.error == EVAL_OK) {
            printf("  x=2: %.6f\n", result.value);
        } else {
            printf("  x=2: ERRO %d\n", result.error);
        }
        
        parser_free_buffer(&tokens);
        parser_free_buffer(&rpn);
    }
    
    return 0;
}
