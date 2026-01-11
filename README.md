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
- Funções: `sin()`, `cos()`, `tan()`, `abs()`, `sqrt()`
- Constantes: `pi`, `e`
- Variáveis: `x`, `theta`, `t`
- Parênteses balanceados
- Suporte a locale (ponto ou vírgula decimal)

### ⏳ Fase 2: RPN (Em Desenvolvimento)
- Algoritmo Shunting Yard de Dijkstra
- Precedência de operadores
- Suporte a funções

### ⏳ Fase 3: Avaliação
- Avaliador de RPN
- Cálculo com valores de variáveis

### ⏳ Fase 4: Interface
- Plotagem de gráficos

## 🚀 Quick Start

### Compilação

```bash
cd /home/hlpp/work/Multicurvas
make clean
make
```

### Execução

```bash
./build/multicurvas
```

### Limpeza

```bash
make clean
```

## 📂 Estrutura do Projeto

```
Multicurvas/
├── src/
│   ├── main.c       # Programa de teste/protótipo
│   ├── parser.c     # Tokenizador e parser
│   └── debug.c      # Funções de debug/visualização
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
