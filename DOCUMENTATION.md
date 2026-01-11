# Multicurvas - Documentação Técnica

## Visão Geral do Projeto

**Objetivo**: Criar um parser e avaliador de expressões matemáticas (função de uma variável) que seja modular e educativo, recriando funcionalmente o programa de plotagem de gráficos do ZX81 em C moderno com RPN (Reverse Polish Notation).

**Fases planejadas**:
1. ✅ Tokenização + Validação
2. ✅ Conversão para RPN (Shunting Yard) - [Ver algoritmo detalhado](SHUNTING_YARD.md)
3. ✅ Avaliador de RPN
4. ✅ Otimizações de Performance (pilha estática, Token compacto)
5. ✅ Benchmark de Performance
6. ⏳ Interface de plotagem

---

## Módulos

### `tokens.h` / `tokens.c` (Conceitual)

**Responsabilidade**: Definir os tipos de tokens e constantes que representam elementos da linguagem matemática.

**Por que modular**: Permite que parser, debug e avaliador usem as mesmas definições de tokens, evitando duplicação e facilitando extensão futura.

#### Tipos Customizados

##### `enum LocaleConfig`
```c
typedef enum {
    LOCALE_POINT,      /* 3.14 (padrão C/EN) */
    LOCALE_COMMA       /* 3,14 (PT-BR, FR, DE) */
} LocaleConfig;
```
- **Finalidade**: Configurar a marca decimal aceita pelo parser
- **Valores**: `LOCALE_POINT` ou `LOCALE_COMMA`
- **Uso**: Passar para `parser_set_locale()`
- **Padrão**: `LOCALE_POINT`

##### `enum TokenType`
```c
typedef enum {
    /* Operadores (0-127, ASCII direto) */
    TOKEN_PLUS       = '+',    /* 43  */
    TOKEN_MINUS      = '-',    /* 45  */
    TOKEN_MULT       = '*',    /* 42  */
    TOKEN_DIV        = '/',    /* 47  */
    TOKEN_POW        = '^',    /* 94  */
    TOKEN_LPAREN     = '(',    /* 40  */
    TOKEN_RPAREN     = ')',    /* 41  */
    
    /* Especiais (>= 128) */
    TOKEN_NUMBER     = 128,    /* Número literal (valor em Token.value) */
    
    /* Variáveis: range 129-138 (10 slots) */
    TOKEN_VARIABLE_X = 129,    /* Variável x */
    TOKEN_VARIABLE_THETA = 130,/* Variável theta */
    TOKEN_VARIABLE_T = 131,    /* Variável t */
    
    /* Constantes: range 140-159 (20 slots) */
    TOKEN_CONST_PI   = 140,    /* Constante π */
    TOKEN_CONST_E    = 141,    /* Constante e */
    
    /* Funções: range 160-199 (40 slots) */
    TOKEN_SIN        = 160,    /* Função sin() */
    TOKEN_COS        = 161,    /* Função cos() */
    TOKEN_TAN        = 162,    /* Função tan() */
    TOKEN_ABS        = 163,    /* Função abs() */
    TOKEN_SQRT       = 164,    /* Função sqrt() */
    TOKEN_EXP        = 165,    /* Função exp() - exponencial (e^x) */
    TOKEN_LOG        = 166,    /* Função log() - logaritmo natural */
    TOKEN_LOG10      = 167,    /* Função log10() - logaritmo base 10 */
    
    TOKEN_END        = 255,    /* Marcador de fim de expressão */
    TOKEN_ERROR      = 256     /* Erro (nunca apareça em output válido) */
} TokenType;
```

- **Estratégia de encoding**:
  - Operadores básicos usam valores ASCII (permite cast direto: `(char)token`)
  - Especiais >= 128 para identificar "bytecodes" da linguagem
  - **Sistema de ranges**: Variáveis (129-138), Constantes (140-159), Funções (160-199)
  - Permite extensibilidade sem modificar funções auxiliares
   (Otimizado)
```c
typedef struct {
    TokenType type;         /* Tipo do token (4 bytes) */
    uint16_t value_index;   /* Índice no array de valores (2 bytes) */
} Token;                    /* Total: 8 bytes (com padding) */
```

- **Finalidade**: Representar um token individual de forma compacta
- **type**: Qual tipo de token é
- **value_index**: Se `type == TOKEN_NUMBER`, índice no array separado de valores; caso contrário, não usado
- **Otimização**: Redução de 16 bytes → 8 bytes por token (50% de economia)
- **Exemplo**: `{TOKEN_NUMBER, 0}` (valor está em `values[0]`) ou `{TOKEN_SIN, 0}`

**Por que separar valores?**
- Maioria dos tokens não tem valores (operadores, funções, variáveis)
- Valores numéricos ficam em array denso separado
- **2x mais tokens** cabem por cache line (8 vs 4)
- **Economia de 38-40%** de memória em expressões típicas
- Melhor locality de referência durante avaliação
- **type**: Qual tipo de token é
- **value**: Se `type == TOKEN_NUMBER`, contém o valor numérico; caso contrário, ignorado
- **Exemplo**: `{TOKEN_NUMBER, 3.14}` ou `{TOKEN_SIN, 0.0}`

---

### `parser.h` / `parser.c`

**Responsabilidade**: 
- Tokenizar strings de expressões matemáticas
- Validar sintaxe básica
- Detectar erros (funções desconhecidas, variáveis misturadas, etc.)
- Converter tokens para RPN (Fase 2)

#### Tipos Customizados

##### `enum ParserError`
```c
typedef enum {
    PARSER_OK = 0,                  /* Sucesso */
    PARSER_UNKNOWN_FUNCTION = 1,    /* Função não reconhecida (ex: "cossecante") */
    PARSER_UNKNOWN_VARIABLE = 2,    /* Variável não reconhecida */
    PARSER_MIXED_VARIABLES = 3,     /* Mistura de variáveis (ex: "x + theta") */
    PARSER_SYNTAX_ERROR = 4,        /* Erro de sintaxe geral (parênteses, etc.) */
    PARSER_MEMORY_ERROR =  (Otimizado)
```c
typedef struct {
    Token *tokens;          /* Array dinâmico de tokens */
    int size;               /* Número de tokens atualmente no buffer */
    int capacity;           /* Espaço alocado (para realocar dinamicamente) */
    
    /* Array separado para valores numéricos (cache-friendly) */
    double *values;         /* Array de valores numéricos */
    int values_size;        /* Número de valores */
    int values_capacity;    /* Capacidade do array de valores */
} TokenBuffer;
```

- **Finalidade**: Container dinâmico para armazenar lista de tokens e valores
- **Gerenciamento**: Parser gerencia alocação/desalocação de ambos os arrays
- **Crescimento**: Tokens começam com 64, values com 16; ambos dobram quando necessário
- **Otimização**: Separação dos valores reduz uso de memória e melhora cache locality
```c
typedef struct {
    Token *tokens;      /* Array dinâmico de tokens */
    int size;           /* Número de tokens atualmente no buffer */
    int capacity;       /* Espaço alocado (para realocar dinamicamente) */
} TokenBuffer;
```

- **Finalidade**: Container dinâmico para armazenar lista de tokens
- **Gerenciamento**: Parser gerencia alocação/desalocação
- **Crescimento**: Começa com 64, dobra quando necessário

#### Variáveis Globais

##### `LocaleConfig parser_locale`
```c
extern LocaleConfig parser_locale;  /* Configuração global no parser.c */
```
- **Padrão**: `LOCALE_POINT`
- **Como trocar**: `parser_set_locale(LOCALE_COMMA);`

#### Funções

##### `void parser_set_locale(LocaleConfig locale)`
- **Objetivo**: Configurar marca decimal globalmente
- **Entrada**: 
  - `locale` (LocaleConfig): `LOCALE_POINT` ou `LOCALE_COMMA`
- **Saída**: Nenhuma (void)
- **Efeito colateral**: Modifica `parser_locale` global
- **Exemplo**:
  ```c
  parser_set_locale(LOCALE_COMMA);
  parser_tokenize("3,14+x", &buf);  // Agora aceita vírgula
  ```

##### `ParserError parser_tokenize(const char *expr, TokenBuffer *output)`
- **Objetivo**: Converter string em lista de tokens
- **Entrada**:
  - `expr` (const char*): String com expressão (ex: `"sin(x)*2+x"`)
  - `output` (TokenBuffer*): Pointer para buffer que receberá tokens
- **Saída**: 
  - `ParserError`: Tipo de sucesso ou erro
  - `output`: Preenchido com tokens se sucesso
- **Características especiais**:
  - **Operadores unários**:
    - `-` (negação): agora representado internamente como um token unário `TOKEN_NEG` (prefix).
      - Critérios para ser unário: início da expressão, após `(`, ou após outro operador/unário (`+`, `-`, `*`, `/`, `^`, `neg`).
      - Implementação: o parser emite `TOKEN_NEG` em contexto unário em vez de inserir `0` antes do `-`. No RPN `TOKEN_NEG` é tratado como uma função unária de alta precedência.
      - Vantagem: encadeamentos como `--x`, `---x`, `-+x` são avaliados corretamente (`--x` = x, `---x` = -x).
    - `+` (positivo/unário): é tratado como no-op (ignorado) quando detectado em contexto unário; `+x`, `(+x)`, `x+ +3` funcionam como esperado.
    - Exemplos: `-x`, `2*(-x)`, `sin(-x)`, `x+-3`, `--x`, `---x` todos são suportados corretamente
- **Validações internas**:
  1. Tokeniza caractere por caractere
  2. Valida variáveis (não mistura x, theta, t)
  3. Valida sintaxe (parênteses balanceados)
  4. Retorna erro se alguma validação falhar
- **Pós-condição**: Se retorna `PARSER_OK`, `output` contém tokens válidos e terminados com `TOKEN_END`
- **Exemplo**:
  ```c
  TokenBuffer buf;
  ParserError err = parser_tokenize("sin(x)", &buf);
  if (err == PARSER_OK) {
      // buf.tokens = [SIN, LPAREN, VAR_X, RPAREN, END]
      debug_print_tokens(&buf);
  }
  ```

##### `ParserError parser_to_rpn(TokenBuffer *tokens, TokenBuffer *rpn)`
- **Objetivo**: Converter tokens infixa para RPN usando algoritmo Shunting Yard
- **Entrada**:
  - `tokens` (TokenBuffer*): Tokens em notação infixa
  - `rpn` (TokenBuffer*): Buffer para receber RPN (será inicializado automaticamente)
- **Saída**: `ParserError` (PARSER_OK, PARSER_MEMORY_ERROR, PARSER_SYNTAX_ERROR)
- **Status**: ✅ Implementado (Shunting Yard de Dijkstra)
- **Algoritmo**: 
  - Usa pilha de operadores e fila de saída
  - Números/variáveis/constantes → saída direta
  - Funções → empilha
  - `(` → empilha
  - `)` → desempilha até `(`, depois aplica função (se houver)
  - Operadores → desempilha por precedência, depois empilha
  - **Precedências**: `^` (4), `*` `/` (3), `+` `-` (2)
  - **Associatividade**: `^` é associativo à direita, outros à esquerda
- **Exemplo**:
  ```c
  TokenBuffer tokens, rpn;
  parser_tokenize("sin(x)*2+x", &tokens);
  parser_to_rpn(&tokens, &rpn);
  // rpn.tokens = [x, sin, 2, *, x, +, END]
  // Equivale a: x sin 2 * x + (tokens e valores)
- **Entrada**: `buf` (TokenBuffer*)
- **Saída**: Nenhuma (void)
- **Crítico**: Chamar após uso para evitar memory leak
- **Nota**: Libera tanto o array de tokens quanto o array de valores
- **Notas**:
  - Aloca memória internamente para `rpn`
  - Sempre chame `parser_free_buffer(&rpn)` após uso
  - Não modifica o buffer de entrada `tokens`

##### `void parser_init_buffer(TokenBuffer *buf)`
- **Objetivo**: Inicializar buffer vazio
- **Entrada**: `buf` (TokenBuffer*) - buffer não inicializado
- **Saída**: Nenhuma (void)
- **Efeito**: Aloca `capacity=64`, `size=0`
- **Nota**: Chamado automaticamente por `parser_tokenize()`, mas útil para uso manual

##### `void parser_free_buffer(TokenBuffer *buf)`
- **Objetivo**: Liberar memória de buffer
- **Entrada**: `buf` (TokenBuffer*)
- **Saída**: Nenhuma (void)
- **Crítico**: Chamar após uso para evitar memory leak
- **Exemplo**:
  ```c
  TokenBuffer buf;
  parser_tokenize("x+1", &buf);
  // ... usar buf ...
  parser_free_buffer(&buf);  // Libera
  ```

##### `int parser_add_token(TokenBuffer *buf, Token token)`
- **Objetivo**: Adicionar token ao buffer (com realocação automática)
- **Entrada**:
  - `buf` (TokenBuffer*): Buffer
  - `token` (Token): Token a adicionar
- **Saída**: 
  - `int`: 1 se sucesso, 0 se erro (memória)
- **Nota**: Função auxiliar, raramente usada diretamente

---

### `debug.h` / `debug.c`

**Responsabilidade**: Funções de inspeção e visualização para debug/aprendizado.

**Observação importante**: O `debug_print_bytecode()` gera bytecode compactado (1 byte por token), mas os valores numéricos de `TOKEN_NUMBER` precisam ser tratados separadamente em uma implementação completa. O bytecode atual é para visualização e aprendizado.

#### Funções

##### `void debug_print_hexdump(const unsigned char *data, int len)`
- **Objetivo**: Exibir bytes em formato hexadecimal (estilo hexdump)
- **Entrada**:
  - `data` (const unsigned char*): Buffer de bytes
  - `len` (int): Número de bytes
- **Saída**: Nenhuma (imprime em stdout)
- **Formato**: 
  ```
  --- HEX DUMP ---
  0000: 96 28 81 29 2A 02 2B 81
  0008: ...
  ```
- **Uso**: Para visualizar bytecode dos tokens (cast para unsigned char*)

##### `void debug_print_tokens(const TokenBuffer *buf)`
- **Objetivo**: Exibir tokens em formato legível
- **Entrada**: `buf` (const TokenBuffer*) - buffer de tokens
- **Saída**: Nenhuma (imprime em stdout)
- **Formato**:
  ```
  --- TOKENS (5) ---
  [ 0] sin          (byte: 150)
  [ 1] (            (byte: 40)
  [ 2] x            (byte: 129)
  [ 3] )            (byte: 41)
  [ 4] END
  ```
- **Uso**: Debug/aprendizado para entender tokenização

##### `void debug_print_bytecode(const TokenBuffer *buf)`
- **Objetivo**: Gerar e exibir bytecode compactado dos tokens (sem padding de struct)
- **Entrada**: `buf` (const TokenBuffer*) - buffer de tokens
- **Saída**: Nenhuma (imprime em stdout)
- **Formato**:
  ```
  --- BYTECODE COMPACTADO ---
  Sequência de bytes: 96 28 81 29 2A 80 2B 81 FF 
  
  Interpretação:
    [0] 0x96 = 150  ← sin
    [1] 0x28 =  40  ← (
    [2] 0x81 = 129  ← x
    [3] 0x29 =  41  ← )
    [4] 0x2A =  42  ← *
    [5] 0x80 = 128  ← NUMBER (valor: 2)
    [6] 0x2B =  43  ← +
    [7] 0x81 = 129  ← x
    [8] 0xFF = 255  ← END
  
  Hex dump compactado:
  0000: 96 28 81 29 2A 80 2B 81 FF
  ```
- **Uso**: Visualizar bytecode real sem overhead de structs (ideal para entender compactação)
- **Diferença**: Extrai apenas os bytes de TokenType, não toda a struct Token (16 bytes → 1 byte por token)

##### `const char* debug_token_name(TokenType type)`
- **Objetivo**: Converter TokenType em string descritiva
- **Entrada**: `type` (TokenType) - tipo do token
- **Saída**: `const char*` - string (ex: "sin", "+", "NUMBER")
- **Exemplo**:
  ```c
  debug_token_name(TOKEN_SIN);    // Retorna "sin"
  debug_token_name(TOKEN_PLUS);   // Retorna "+"
  debug_token_name(TOKEN_NUMBER); // Retorna "NUMBER"
  ```

---

### `evaluator.h` / `evaluator.c`

**Responsabilidade**: Avaliar expressões em RPN (Reverse Polish Notation) e calcular resultados numéricos.

#### Tipos Customizados

##### `enum EvalError`
```c
typedef enum {
    EVAL_OK = 0,
    EVAL_STACK_ERROR,           /* Pilha vazia ou múltiplos valores no final */
    EVAL_DIVISION_BY_ZERO,      /* Divisão por zero */
    EVAL_DOMAIN_ERROR,          /* Domínio inválido */
    EVAL_MATH_ERROR             /* Overflow, NaN */
} EvalError;
```
- **Finalidade**: Identificar tipo de erro durante avaliação
- **EVAL_DIVISION_BY_ZERO**: Separado para permitir estratégias especiais (limite, stencil, marcar descontinuidade)
- **EVAL_DOMAIN_ERROR**: sqrt negativo, log≤0, asin/acos fora de [-1,1], etc.

##### `struct EvalResult`
```c
typedef struct {
    EvalError error;
    double value;
} EvalResult;
```
- **Finalidade**: Retornar resultado e status de erro
- **value**: Válido apenas se `error == EVAL_OK`

#### Funções

##### `EvalResult evaluator_eval_rpn(const TokenBuffer *rpn, double var_value)`
- **Objetivo**: Avaliar expressão RPN com valor para a variável
- **Entrada**:
  - `rpn` (const TokenBuffer*): Expressão em RPN
  - `var_value` (double): Valor para x, theta ou t
- **Saída**: `EvalResult` (erro + valor)
- **Otimização**: Usa pilha **estática** de 64 níveis (sem malloc/free por avaliação)
  - Overhead: ~512 bytes na stack
  - Performance: **2-3x mais rápido** que com alocação dinâmica
  - Suficiente para expressões com até 64 níveis de profundidade (mais que adequado)
- **Algoritmo**:
  1. Cria pilha estática de doubles `[64]`
  2. Para cada token:
     - Número → busca valor em `rpn->values[token.value_index]`, empilha
     - Variável → empilha var_value
     - Constante (pi, e) → empilha valor
     - Operador → desempilha 2, calcula, empilha
     - Função → desempilha 1, calcula, empilha
  3. Retorna valor final (deve sobrar exatamente 1 na pilha)
- **Exemplo**:
  ```c
  TokenBuffer rpn;
  parser_tokenize("sin(x)*2+1", &tokens);
  parser_to_rpn(&tokens, &rpn);
  
  EvalResult result = evaluator_eval_rpn(&rpn, 1.0);  // x=1
  if (result.error == EVAL_OK) {
      printf("Resultado: %g\n", result.value);  // 2.68294
  }
  ```
- **Funções suportadas** (20 total):
  - Trigonométricas: sin, cos, tan
  - Inversas: asin, acos, atan
  - Hiperbólicas: sinh, cosh, tanh
  - Hiperbólicas inversas: asinh, acosh, atanh
  - Exponencial: exp (e^x)
  - Logaritmos: log (ln), log10
  - Outras: abs, sqrt, ceil, floor, frac

---

### `main.c`

**Responsabilidade**: Programa de teste/protótipo que demonstra o parser em ação.

**Estrutura**:
1. Função auxiliar `test_expression(const char *expr)` que:
   - Tokeniza a expressão
   - Exibe tokens em formato legível
   - Exibe bytecode compactado
   - Tenta conversão para RPN (stub)
   - Trata erros

2. Função `main()` que:
   - Executa testes com `LOCALE_POINT`
   - Executa testes com `LOCALE_COMMA`
   - Testa casos de erro

**Como estender**: Adicione novas chamadas a `test_expression()` com novos casos de teste.

---

### Build & Test Framework (PR #1)

O repositório recebeu uma pequena reestruturação para facilitar testes automatizados e o fluxo de build.

- `Makefile` refatorado: alvos unificados e fluxo de testes integrado. Use `make all` para compilar e rodar a suíte demonstrativa.
- Nova pasta `test/`: contém programas de teste (ex.: `test/unary.c`, `test/benchmark.c`, `test/memory.c`). A fonte principal (`src/`) continua a conter a implementação, enquanto `test/` agrega executáveis de verificação.
- Targets relevantes (exemplos):
  - `make all` — compila e executa a suíte de testes demonstrativa (tokenização, RPN, avaliação, análise de memória).
  - `make benchmark` — compila e executa o benchmark de performance.
  - `make memtest` — executa o programa de análise de memória.
  - `make run` — executa o binário principal (quando aplicável).

Como rodar localmente (exemplo):
```bash
# compila e roda a suíte demonstrativa
make all

# apenas benchmark
make benchmark

# apenas análise de memória
make memtest
```

Essas alterações melhoram a testabilidade do projeto e facilitam integração contínua futura.

## Fluxo de Dados

```
String de entrada
       ↓
parser_tokenize()
       ↓
[Validações] → Erro → return PARSER_ERROR
       ↓
TokenBuffer com tokens
       ↓
debug_print_tokens() [opcional]
       ↓
parser_to_rpn() [Fase 2]
       ↓
TokenBuffer em RPN
       ↓
evaluator_rpn() [Fase 3]
       ↓
double resultado
```

---

## Otimizações de Performance

### 1. Token Compacto (Economia de Memória)

**Problema**: Estrutura original desperdiçava memória
```c
/* Antes: 16 bytes por token */
struct TokenOld {
    TokenType type;    // 4 bytes
    double value;      // 8 bytes
};                     // Total: 16 bytes (com padding)
```

Para expressão `sin(x) + 2 * 3.14` (9 tokens, apenas 2 números):
- Memória antiga: 144 bytes
- Desperdício: 7 tokens × 8 bytes = 56 bytes de doubles não usados

**Solução**: Separar valores em array dedicado
```c
/* Depois: 8 bytes por token */
struct Token {
    TokenType type;        // 4 bytes
    uint16_t value_index;  // 2 bytes
};                         // Total: 8 bytes (com padding)
```

Array separado:
```c
TokenBuffer {
    Token tokens[9];      // 72 bytes
    double values[2];     // 16 bytes
};                        // Total: 88 bytes
```

**Resultados**:
- **Redução por token**: 50% (16 → 8 bytes)
- **Economia em expressões típicas**: 38-40%
- **Tokens por cache line**: 2x mais (8 vs 4 tokens/64 bytes)
- **Benefício**: Melhor locality de referência durante avaliação em loops

### 2. Pilha Estática de Avaliação

**Problema**: `malloc/free` em cada avaliação
```c
/* Antes: alocação dinâmica */
double *stack = malloc(rpn->size * sizeof(double));
// ... avalia expressão ...
free(stack);
```

Em loops de 10 milhões de iterações:
- 10M × (malloc + free) = overhead significativo
- Fragmentação de memória
- Cache misses

**Solução**: Pilha estática de tamanho fixo
```c
/* Depois: array estático */
#define MAX_EVAL_STACK_SIZE 64
double stack[MAX_EVAL_STACK_SIZE];  // ~512 bytes na stack
```

**Resultados**:
- **Performance**: 2-3x mais rápido em loops intensivos
- **Overhead**: ~512 bytes (64 × 8 bytes) - aceitável
- **Capacidade**: 64 níveis de profundidade - mais que suficiente
  - Expressão `x * exp(x)` usa apenas 3 níveis
  - Expressões práticas raramente excedem 10 níveis
- **Sem fragmentação**: Memória na stack é automaticamente gerenciada
- **Sem falhas de alocação**: Sem risco de malloc retornar NULL

### 3. Função exp() Nativa

**Problema**: Usar `e^x` via potenciação é menos eficiente
```c
/* Antes: usando constante e + pow */
e^x  →  TOKEN_CONST_E, TOKEN_VARIABLE_X, TOKEN_POW
     →  avalia: pow(2.718..., x)
```

**Solução**: Função `exp()` nativa da math.h
```c
/* Depois: função dedicada */
exp(x)  →  TOKEN_EXP, TOKEN_VARIABLE_X
        →  avalia: exp(x)  // Otimizada pela libm
```

**Resultados** (benchmark de integração, 10M pontos):
- `e^x`: Overhead de 3.95x vs hardcoded
- `exp(x)`: Overhead de 2.56x vs hardcoded
- **Melhoria**: 35% mais rápido!

### Benchmark Completo

Programa: `make benchmark`

Teste: Integração numérica de `f(x) = x * exp(x)` de 0 a 1 com 10 milhões de pontos

**Resultados** (10M iterações):
```
Parsing:              0.000004s (negligível)
Hardcoded function:   0.039s
Parsed function:      0.101s
Overhead parseado:    2.56x
```

**Análise**:
- Parsing é **instantâneo** (4 microssegundos)
- Overhead de apenas **2.56x** é excelente para um interpretador
- Pilha estática contribui significativamente para a performance
- Token compacto melhora cache hits durante avaliação

---

## Algoritmos Implementados

### Shunting Yard (Dijkstra)

Para uma explicação detalhada e completa do algoritmo Shunting-Yard com exemplos passo a passo, consulte: **[SHUNTING_YARD.md](SHUNTING_YARD.md)**

---

## Fluxo de Dados

```
String de entrada
       ↓
parser_tokenize()
       ↓
[Validações] → Erro → return PARSER_ERROR
       ↓
TokenBuffer com tokens
       ↓
debug_print_tokens() [opcional]
       ↓
parser_to_rpn() [Fase 2]
       ↓
TokenBuffer em RPN
       ↓
evaluator_rpn() [Fase 3]
       ↓
double resultado
```

---

## Checklist de Funcionalidades

### ✅ Fase 1: Tokenização (Completa)
- [x] Tokenização de operadores básicos (+, -, *, /, ^)
- [x] **Suporte a operador unário** - (negativo)
- [x] Tokenização de números (com suporte a locale)
- [x] Tokenização de funções (20 funções matemáticas)
- [x] Tokenização de constantes (pi, e)
- [x] Tokenização de variáveis (x, theta, t)
- [x] Detecção de parênteses
- [x] Validação de variáveis (não mista)
- [x] Validação de sintaxe básica
- [x] Suporte a locale (ponto e vírgula decimal)
- [x] Debug output (hex, tokens)

### Fase 2: RPN ✅
- [x] Algoritmo Shunting Yard
- [x] Suporte a precedência de operadores
- [x] Suporte a associatividade (^ é associativo à direita)
- [x] Suporte a funções

### Fase 3: Avaliação ✅
- [x] Avaliador de RPN (pilha estática)
- [x] Substituição de variáveis
- [x] Cálculo de constantes (pi, e)
- [x] Tratamento de erros matemáticos (divisão por zero, domínio, overflow)
- [x] 20 funções implementadas

### Fase 4: Otimizações ✅
- [x] Token compacto (8 bytes)
- [x] Pilha estática de avaliação
- [x] Função exp() nativa

### Fase 5: Benchmark ✅
- [x] Integração numérica (10M pontos)
- [x] Comparação hardcoded vs parseado
- [x] Análise de memória

### Fase 6: Interface de Plotagem ⏳
- [ ] Plotagem de gráficos 2D
- [ ] Coordenadas retangulares, polares, paramétricas
- [ ] Detecção de descontinuidades

---

## Notas de Desenvolvimento

### Convenções de Código
- Funções privadas (internas a um módulo): `static`
- Funções públicas (header .h): Sem `static`
- Variáveis globais: Poucas e bem documentadas (ex: `parser_locale`)
- Erro/sucesso: Enums `*Error`, retorno `0` ou enums

### Memory Management
- Sempre liberar com `parser_free_buffer()` após uso
- Parser aloca internamente, usuário não precisa alocar TokenBuffer

### Extensão Futura

**Sistema de ranges para extensibilidade:**

O projeto usa **ranges de valores** para permitir adição de funções, variáveis e constantes sem modificar a lógica de parsing:

```c
/* Ranges definidos em tokens.h */
#define TOKEN_VARIABLE_START  129
#define TOKEN_VARIABLE_END    138   /* 10 slots disponíveis */

#define TOKEN_CONST_START     140
#define TOKEN_CONST_END       159   /* 20 slots disponíveis */

#define TOKEN_FUNCTION_START  160
#define TOKEN_FUNCTION_END    199   /* 40 slots disponíveis */
```

**Para adicionar nova função** (ex: "log"):
1. Em `tokens.h`: `TOKEN_LOG = 165` (dentro do range 160-199)
2. Em `parser.c`: `CHECK_KEYWORD("log", TOKEN_LOG)`
3. Em `debug.c`: Case no `debug_token_name()`
4. No avaliador (Fase 3): Case para calcular `log()`

**Não precisa modificar**: `is_function()`, `is_variable()`, `is_constant()` - usam ranges!

**Para adicionar nova variável** (ex: "r"):
1. Em `tokens.h`: `TOKEN_VARIABLE_R = 132` (dentro do range 129-138)
2. Em `parser.c`: `CHECK_KEYWORD("r", TOKEN_VARIABLE_R)`
3. Em `debug.c`: Case no `debug_token_name()`

**Para adicionar nova constante** (ex: "phi" = número de ouro):
1. Em `tokens.h`: `TOKEN_CONST_PHI = 142` (dentro do range 140-159)
2. Em `parser.c`: `CHECK_KEYWORD("phi", TOKEN_CONST_PHI)`
3. Em `debug.c`: Case no `debug_token_name()`
4. No avaliador (Fase 3): Retornar valor `1.618033988749...`

---

**Última atualização**: 2026-01-11
