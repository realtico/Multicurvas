# Multicurvas

Um parser e avaliador de expressões matemáticas em C, recriando funcionalmente o programa de plotagem de gráficos do ZX81 (computador de 8-bits dos anos 80) em C moderno com arquitetura modular.

## 🎯 Objetivo

Implementar um sistema que permita:
- **Tokenização** de expressões matemáticas com suporte a múltiplas variáveis
- **Validação** de sintaxe e consistência
- **Conversão para RPN** (Reverse Polish Notation) para avaliação eficiente
- **Plotagem** de gráficos 2D (polares, retangulares, paramétricos)

## 📋 Funcionalidades Suportadas

### ✅ Fase 1: Tokenização (Completa)
- Operadores: `+`, `-`, `*`, `/`, `^`
- **Operador unário `-`** (negativo): `-x`, `2*(-x)`, `sin(-x)` funcionam corretamente
- **20 Funções**: `sin`, `cos`, `tan`, `abs`, `sqrt`, `exp`, `log`, `log10`, `sinh`, `cosh`, `tanh`, `asin`, `acos`, `atan`, `asinh`, `acosh`, `atanh`, `ceil`, `floor`, `frac`
- Constantes: `pi`, `e`
- Variáveis: `x`, `theta`, `t`
- Parênteses balanceados
- Suporte a locale (ponto ou vírgula decimal)
- Sistema de ranges para extensibilidade (10 variáveis, 20 constantes, 40 funções)

### ✅ Fase 2: RPN (Completa)
- Algoritmo Shunting Yard de Dijkstra - [Explicação detalhada](SHUNTING_YARD.md)
- Precedência de operadores
- Associatividade (^ à direita, outros à esquerda)
- Suporte a funções

### ✅ Fase 3: Avaliação (Completa)
- Avaliador de RPN com pilha estática de doubles (otimizado)
- Suporte a todas as 20 funções matemáticas
- Tratamento específico de erros:
  - `EVAL_DIVISION_BY_ZERO` - permite estratégias de limite/stencil
  - `EVAL_DOMAIN_ERROR` - domínio inválido (sqrt negativo, log≤0, etc.)
  - `EVAL_MATH_ERROR` - overflow, NaN
  - `EVAL_STACK_ERROR` - expressão mal-formada
- Substituição de variáveis em tempo de avaliação

### ✅ Fase 4: Otimizações (Completa)
- **Token compacto**: 50% de redução (16→8 bytes), 38% economia total
- **Pilha estática**: 2-3x mais rápido, sem malloc/free por avaliação
- **Cache-friendly**: 2x mais tokens por cache line
- **Função exp() nativa**: 35% mais rápida que e^x

### ✅ Fase 5: Benchmark (Completa)
- Integração numérica de 10M pontos
- Comparação hardcoded vs parseado
- Overhead de apenas **2.56x** (excelente!)
- Ferramentas de análise de memória

### ⏳ Próximos Passos

#### Fase 6: Interface de Plotagem
- Plotagem de gráficos 2D
- Suporte a coordenadas retangulares, polares e paramétricas
- Detecção de descontinuidades (divisão por zero)

## 🚀 Quick Start

### Compilação

```bash
cd /home/hlpp/work/Multicurvas
make clean
make all
```

### Execução

**Testes do parser:**
```bash
./build/multicurvas
```

**Benchmark de performance:**
```bash
./build/benchmark
```

**Análise de memória:**
```bash
./build/memory_test
```

### Limpeza

```bash
make clean
```

## 📂 Estrutura do Projeto

```
Multicurvas/
├── src/
│   ├── main.c           # Programa de teste/protótipo
>>>>>>> ab2979c (Fix: Corrige erros de formatação nos arquivos .md)
│   ├── main_benchmark.c # Benchmark de performance
│   ├── benchmark.c      # Testes de integração numérica
│   ├── memory_test.c    # Análise de uso de memória
│   ├── parser.c         # Tokenizador e parser
│   ├── evaluator.c      # Avaliador de RPN
│   └── debug.c          # Funções de debug/visualização
├── include/
│   ├── tokens.h         # Definições de tokens
│   ├── parser.h         # Interface do parser
│   ├── evaluator.h      # Interface do avaliador
│   └── debug.h          # Funções de debug
├── build/               # Arquivos compilados (gerado)
├── Makefile             # Automação de compilação
├── .gitignore           # Exclusões do Git
├── README.md            # Este arquivo
├── DOCUMENTATION.md     # Documentação técnica detalhada
└── SHUNTING_YARD.md     # Explicação do algoritmo RPN
```

## 📚 Documentação

- **[DOCUMENTATION.md](DOCUMENTATION.md)** - Documentação técnica completa
- **[SHUNTING_YARD.md](SHUNTING_YARD.md)** - Explicação detalhada do algoritmo de conversão RPN

A documentação inclui:
- Responsabilidade de cada módulo
- Tipos customizados e seus valores esperados
- Descrição de cada função (entrada, saída, exemplos)
- Otimizações de performance implementadas
- Fluxo de dados
- Guia de extensão

## 🚀 Performance

**Benchmark**: Integração numérica de `f(x) = x * exp(x)` com 10 milhões de pontos

| Método | Tempo | Overhead |
|--------|-------|----------|
| Parsing | 0.000004s | - |
| Hardcoded | 0.039s | 1.0x |
| Parseado | 0.101s | 2.56x |

**Otimizações implementadas:**
- Pilha estática sem malloc/free: 2-3x mais rápido
- Token compacto (8 vs 16 bytes): 50% menos memória, melhor cache
- Função exp() nativa: 35% mais rápida que e^x   # Funções de debug/visualização
├── include/
│   ├── tokens.h     # Definições de tokens
│   ├── parser.h     # Interface do parser
│   └── debug.h      # Funções de debug
├── build/           # Arquivos compilados (gerado)
├── Makefile         # Automação de compilação
├── .gitignore       # Exclusões do Git
└── DOCUMENTATION.md # Documentação técnica detalhada
```

## 📚 Documentação

Consulte [DOCUMENTATION.md](DOCUMENTATION.md) para documentação técnica completa com:
- Responsabilidade de cada módulo
- Tipos customizados e seus valores esperados
- Descrição de cada função (entrada, saída, exemplos)
- Fluxo de dados
- Guia de extensão

## 💡 Exemplos de Uso

### Expressão Retangular

```c
test_expression("sin(x)*2+x");     // sin(x)·2 + x
```

### Expressão Polar

```c
test_expression("9*(theta-pi/2)");  // 9·(θ - π/2)
```

### Expressão Paramétrica

```c
test_expression("2*e^(-t/2)");      // 2·e^(-t/2)
```

## 🔧 Configuração de Locale

Por padrão, o parser usa **ponto decimal** (`.`):

```c
parser_set_locale(LOCALE_POINT);   // 3.14
parser_set_locale(LOCALE_COMMA);   // 3,14
```

## 📝 Histórico & Motivação

Este projeto reconstrói um programa que o autor criou aos 11 anos em um ZX81, que avaliava stringas de expressões matemáticas de forma similar às lambdas do Python. Aos 16 anos tentou recriar em C, mas trilhou caminho errado. Agora usa aprendizado incremental, arquitetura modular e RPN como abordagem correta.

## 🛠️ Dependências

- Compilador C99 (gcc ou clang)
- Make
- Biblioteca matemática padrão C (libm)

## ⚠️ Avisos

- ⚠️ Não suporta múltiplas variáveis na mesma expressão (ex: `x+theta` gerará erro)
- ⚠️ Funções desconhecidas resultam em erro de parsing
- ⚠️ Sintaxe inválida (parênteses desbalanceados) resulta em erro

## 🎓 Como Contribuir / Aprender

Este é um projeto educacional. Para adicionar nova funcionalidade:

1. Atualize [DOCUMENTATION.md](DOCUMENTATION.md) com plano
2. Modifique os tipos em `include/tokens.h`
3. Implemente em `src/parser.c` (ou módulo apropriado)
4. Adicione testes em `src/main.c`
5. Compile e valide: `make run`

## 📄 Licença

[Escolha uma licença - MIT, GPL3, etc]

## 👤 Autor

- **Nome**: Hardy Pinto (Realtico)
- **Data de início**: 2026-01-11
- **Status**: Em desenvolvimento ativo

---

**Nota**: Este projeto é um exercício de aprendizado contínuo. Sinta-se livre para questionar, sugerir melhorias e experimentar!
