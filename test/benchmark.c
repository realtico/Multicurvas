#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "parser.h"
#include "evaluator.h"

/* Função hardcoded: f(x) = x * e^x */
static double hardcoded_function(double x) {
    return x * x + 1;
}

/* Integração numérica usando método do trapézio */
static double integrate_hardcoded(double a, double b, int n_steps) {
    double h = (b - a) / n_steps;
    double sum = 0.5 * (hardcoded_function(a) + hardcoded_function(b));
    
    for (int i = 1; i < n_steps; i++) {
        double x = a + i * h;
        sum += hardcoded_function(x);
    }
    
    return sum * h;
}

/* Integração usando função parseada */
static double integrate_parsed(const TokenBuffer *rpn, double a, double b, int n_steps) {
    double h = (b - a) / n_steps;
    
    EvalResult result_a = evaluator_eval_rpn(rpn, a);
    EvalResult result_b = evaluator_eval_rpn(rpn, b);
    
    if (result_a.error != EVAL_OK || result_b.error != EVAL_OK) {
        return 0.0;
    }
    
    double sum = 0.5 * (result_a.value + result_b.value);
    
    for (int i = 1; i < n_steps; i++) {
        double x = a + i * h;
        EvalResult result = evaluator_eval_rpn(rpn, x);
        if (result.error != EVAL_OK) {
            return 0.0;
        }
        sum += result.value;
    }
    
    return sum * h;
}

/* Calcula tempo em segundos */
static double get_time_diff(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

void run_benchmark(void) {
    const char *expression = "x * x +1";
    const double a = 0.0;
    const double b = 1.0;
    const int n_steps = 10000000;  /* 10 milhões de pontos */
    
    printf("=== BENCHMARK: Integração de f(x) = x * e^x de %.1f a %.1f ===\n\n", a, b);
    printf("Número de pontos: %d\n", n_steps);
    printf("Expressão: %s\n\n", expression);
    
    /* FASE 1: Parsing (feito apenas uma vez) */
    printf("--- FASE 1: Parsing (overhead único) ---\n");
    clock_t parse_start = clock();
    
    TokenBuffer tokens;
    ParserError err = parser_tokenize(expression, &tokens);
    if (err != PARSER_OK) {
        printf("Erro no parsing: %d\n", err);
        return;
    }
    
    TokenBuffer rpn;
    err = parser_to_rpn(&tokens, &rpn);
    if (err != PARSER_OK) {
        printf("Erro na conversão RPN: %d\n", err);
        parser_free_buffer(&tokens);
        return;
    }
    
    clock_t parse_end = clock();
    double parse_time = get_time_diff(parse_start, parse_end);
    printf("Tempo de parsing: %.6f segundos\n\n", parse_time);
    
    /* FASE 2: Integração com função hardcoded */
    printf("--- FASE 2: Integração (função hardcoded) ---\n");
    clock_t hardcoded_start = clock();
    double result_hardcoded = integrate_hardcoded(a, b, n_steps);
    clock_t hardcoded_end = clock();
    double hardcoded_time = get_time_diff(hardcoded_start, hardcoded_end);
    
    printf("Resultado: %.10f\n", result_hardcoded);
    printf("Tempo: %.6f segundos\n\n", hardcoded_time);
    
    /* FASE 3: Integração com função parseada */
    printf("--- FASE 3: Integração (função parseada) ---\n");
    clock_t parsed_start = clock();
    double result_parsed = integrate_parsed(&rpn, a, b, n_steps);
    clock_t parsed_end = clock();
    double parsed_time = get_time_diff(parsed_start, parsed_end);
    
    printf("Resultado: %.10f\n", result_parsed);
    printf("Tempo: %.6f segundos\n\n", parsed_time);
    
    /* ANÁLISE */
    printf("=== ANÁLISE ===\n");
    printf("Diferença de resultados: %.2e (erro relativo)\n", 
           fabs(result_hardcoded - result_parsed) / fabs(result_hardcoded));
    printf("Overhead da avaliação parseada: %.2fx\n", parsed_time / hardcoded_time);
    printf("Custo do parsing: %.2f%% do tempo total parseado\n", 
           (parse_time / (parse_time + parsed_time)) * 100);
    printf("\nValor esperado (analítico): %.10f\n", 1.0);  /* Integral de x*e^x de 0 a 1 = 1 */
    
    /* Cleanup */
    parser_free_buffer(&tokens);
    parser_free_buffer(&rpn);
}


int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║      MULTICURVAS - Benchmark de Performance              ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    parser_set_locale(LOCALE_POINT);
    run_benchmark();
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                  Benchmark Completo                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    return 0;
}
