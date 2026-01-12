/* Parser secundário para a aplicação `multicurvas`.
 * Define a estrutura `Plot` e funções para preparar dados de plotagem.
 *
 * FLUXO DE USO:
 * 1. Usuário fornece string: "Y=sin(x):-3,3:" ou "X=cos(t);Y=sin(t):0,6:"
 * 2. Chama plot_parse_text() → retorna struct Plot com tipo, expressões e intervalo
 * 3. Chama plot_generate_samples() → compila expressões e gera buffer de pontos (x,y)
 * 4. Usa os dados para renderizar (SDL, terminal, arquivo, etc.)
 * 5. Libera tudo com plot_free()
 */
#ifndef MULTICURVAS_PLOT_H
#define MULTICURVAS_PLOT_H

#include <stddef.h>

#define PLOT_DEFAULT_SAMPLES 500

typedef enum {
    PLOT_UNKNOWN = 0,
    PLOT_CARTESIAN,   /* Y = f(x) */
    PLOT_POLAR_R,     /* R = f(t) */
    PLOT_POLAR_R2,    /* R**2 = f(t) */
    PLOT_PARAMETRIC   /* X = f(t), Y = f(t) */
} PlotType;

typedef enum {
    PLOT_STATUS_OK = 0,
    PLOT_STATUS_PARSE_ERROR,
    PLOT_STATUS_INVALID_INTERVAL
} PlotStatus;

typedef struct Plot {
    PlotType type;
    char *expr1;    /* Para cartesiano: Y; polar: R ou R**2; paramétrico: X */
    char *expr2;    /* Para paramétrico: Y. Caso contrário NULL */
    double C;       /* Início do domínio/parâmetro */
    double D;       /* Fim do domínio/parâmetro */
    int has_interval;
    int samples;    /* número de amostras (padrão: PLOT_DEFAULT_SAMPLES) */
} Plot;

/* Buffer de dados prontos para plotagem */
typedef struct PlotData {
    double *x;      /* Coordenadas X dos pontos */
    double *y;      /* Coordenadas Y dos pontos */
    int *status;    /* Status de cada ponto (0=OK, 1=erro) */
    int count;      /* Número de pontos válidos */
    int capacity;   /* Tamanho alocado dos arrays */
} PlotData;

/* Analisa a string de entrada e aloca um `Plot`.
 * Retorna Plot alocado ou NULL em caso de erro.
 * Se errmsg não for NULL, grava mensagem de erro (caller deve liberar).
 */
Plot *plot_parse_text(const char *input, char **errmsg);

/* Libera um `Plot` retornado por `plot_parse_text`. */
void plot_free(Plot *p);

/* Gera dados de plotagem a partir de um Plot.
 * - Compila as expressões usando o parser/avaliador existente
 * - Gera samples pontos no intervalo [C,D]
 * - Avalia as expressões e preenche arrays x,y
 * - Marca pontos com erro de avaliação (divisão por zero, domínio, etc.)
 * Retorna PlotData alocado ou NULL em caso de erro.
 */
PlotData *plot_generate_samples(const Plot *plot, char **errmsg);

/* Libera um PlotData retornado por plot_generate_samples. */
void plot_data_free(PlotData *data);

#endif /* MULTICURVAS_PLOT_H */
