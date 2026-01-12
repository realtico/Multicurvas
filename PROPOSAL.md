

# Otimização por Tabela de Dispatch — Proposta

Status: rascunho

Contexto
--------
- Mudanças recentes introduziram um token unário (`TOKEN_NEG`) e refatoraram o laço do avaliador. Benchmarks medidos mostram pequeno aumento no overhead interpretado após micro-otimizações.
- Foi explorado um refactor baseado em `switch` e a redução do layout de `Token` (por exemplo para `uint8_t`), com ganhos observáveis.

Objetivo
--------
Documentar uma otimização não intrusiva e futura: substituir o `switch`/cadeia de ifs do loop crítico de avaliação por uma tabela de dispatch (ponteiros para funções) indexada por `token.type`.

Justificativa
-----------
- Diminuir mispredições de branch no laço quente do avaliador.
- Fornecer handlers pequenos por token (push número, aplicar `+`, chamar `sin`, etc.) que operem diretamente sobre um buffer de pilha explícito.
- Manter o código legível e educacional: handlers são funções curtas e explícitas; otimizações de baixo nível podem ser feitas em branches separados.

Design de alto nível
-------------------
- Uma tabela `TokenHandler handlers[256]` é populada na inicialização.
- Assinatura de `TokenHandler` (exemplo):

```c
typedef EvalError (*TokenHandler)(double *stack, int *stack_top,
                                  const Token *token,
                                  const TokenBuffer *rpn,
                                  double var_value);
```

- Cada handler manipula `stack` e `stack_top` diretamente e retorna um `EvalError`.
- O laço do avaliador passa a: carregar token, buscar handler, chamar handler. Handlers para operações simples são curtos e previsíveis.

Exemplos de handlers (esboço)
-----------------------------

```c
static EvalError handle_number(double *stack, int *stack_top, const Token *token, const TokenBuffer *rpn, double var_value) {
    if (*stack_top >= MAX_EVAL_STACK_SIZE - 1) return EVAL_STACK_ERROR;
    stack[++(*stack_top)] = rpn->values[token->value_index];
    return EVAL_OK;
}

static EvalError handle_plus(double *stack, int *stack_top, const Token *token, const TokenBuffer *rpn, double var_value) {
    if (*stack_top < 1) return EVAL_STACK_ERROR;
    double b = stack[(*stack_top)--];
    double a = stack[(*stack_top)--];
    stack[++(*stack_top)] = a + b;
    return EVAL_OK;
}
```

Notas de integração
-------------------
- Inicializar `handlers` na inicialização do programa (ou de forma lazy) e registrar handlers para números, variáveis, constantes, operadores binários, `neg` unário e funções.
- Documentar a inicialização de `handlers` em `DOCUMENTATION.md` como opção de otimização.
- A abordagem adiciona uma chamada indireta por token; em CPUs modernas uma tabela densa e quente tende a estabilizar previsões e reduzir custo de mispredição comparado a múltiplos `if`.

Prós e Contras
--------------
- Prós:
  - Redução potencial de mispredições no laço quente
  - Handlers encapsulados, pontos claros de especialização
  - Facilidade para estender com novos tokens
- Contras:
  - Indireção extra (chamada por ponteiro de função) — impacto depende de CPU e layout de código
  - Mais unidades de código para manter (muitos handlers pequenos)
  - Se necessário, manter uma versão `switch` para legibilidade e fornecer a variante por flag de compilação

Links
-----
- Documentação principal: [DOCUMENTATION.md](DOCUMENTATION.md)
- Artefatos de benchmark: `build/benchmark` e `build/benchmark.test`

Próximos passos (se decidirmos implementar)
------------------------------------------
1. Prototipar handlers para tokens quentes: `NUMBER`, `VARIABLE_*`, `CONST_*`, `+ - * / ^`, `NEG`, `SIN/COS/EXP`.
2. Medir e comparar contra o avaliador baseado em `switch` na(s) máquina(s) alvo.
3. Se vantajoso, fornecer a variante de dispatch atrás de uma flag de compilação (por ex. `#define USE_DISPATCH`) e manter `switch` por compatibilidade/legibilidade.

Observações
-----------
- Esta proposta documenta uma possibilidade de otimização com objetivos educacionais e de manutenção. Se desejar, podemos prototipar rapidamente o handler para `NUMBER` e `+` e rodar um benchmark local para comparar.

