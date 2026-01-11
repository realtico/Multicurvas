# Multicurvas - Documentação Técnica

## Visão Geral do Projeto

**Objetivo**: Criar um parser e avaliador de expressões matemáticas (função de uma variável) que seja modular e educativo, recriando funcionalmente o programa de plotagem de gráficos do ZX81 em C moderno com RPN (Reverse Polish Notation).

**Fases planejadas**:
1. ✅ Tokenização + Validação
2. ⏳ Conversão para RPN
3. ⏳ Avaliador de RPN
4. ⏳ Interface de plotagem

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
    TOKEN_VARIABLE_X = 129,    /* Variável x */
    TOKEN_VARIABLE_THETA = 130,/* Variável theta */
    TOKEN_VARIABLE_T = 131,    /* Variável t */
    TOKEN_CONST_PI   = 140,    /* Constante π */
    TOKEN_CONST_E    = 141,    /* Constante e */
    TOKEN_SIN        = 150,    /* Função sin() */
    TOKEN_COS        = 151,    /* Função cos() */
    TOKEN_TAN        = 152,    /* Função tan() */
    TOKEN_ABS        = 153,    /* Função abs() */
    TOKEN_SQRT       = 154,    /* Função sqrt() */
    TOKEN_END        = 255,    /* Marcador de fim de expressão */
    TOKEN_ERROR      = 256     /* Erro (nunca apareça em output válido) */
} TokenType;
```

- **Estratégia de encoding**:
  - Operadores básicos usam valores ASCII (permite cast direto: `(char)token`)
  - Especiais >= 128 para identificar "bytecodes" da linguagem
  - Compacta a string original em tokens de byte
  
- **Padrão que será usado**: Exemplo: `"sin(x)*2+x"` → `[SIN, 150][LPAREN, 40][VARIABLE_X, 129][RPAREN, 41][MULT, 42][NUMBER, 2][PLUS, 43][VARIABLE_X, 129][END, 255]`

##### `struct Token`
```c
typedef struct {
    TokenType type;     /* Tipo do token */
    double value;       /* Valor (apenas para TOKEN_NUMBER) */
} Token;
```

- **Finalidade**: Representar um token individual
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
    PARSER_MEMORY_ERROR = 5         /* Falha ao alocar memória */
} ParserError;
```

- **Quando retorna cada erro**:
  - `PARSER_UNKNOWN_FUNCTION`: `"cossecante(x)"`, `"log(x)"` (não suportadas)
  - `PARSER_UNKNOWN_VARIABLE`: Variável que não é x, theta ou t
  - `PARSER_MIXED_VARIABLES`: `"x + theta"` (mesma expressão não pode usar múltiplas variáveis)
  - `PARSER_SYNTAX_ERROR`: `"sin(x))"` (parênteses desbalanceados), `"3++5"` (operador duplo)
  - `PARSER_MEMORY_ERROR`: Sem memória para alocar buffer

##### `struct TokenBuffer`
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
- **Objetivo**: Converter tokens infix para RPN (Shunting Yard algorithm - Fase 2)
- **Entrada**:
  - `tokens` (TokenBuffer*): Tokens em notação infixa
  - `rpn` (TokenBuffer*): Buffer para receber RPN
- **Saída**: `ParserError`
- **Status**: ⏳ Stub (copia tokens por enquanto)
- **Algoritmo planejado**: Shunting Yard de Dijkstra

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

### `main.c`

**Responsabilidade**: Programa de teste/protótipo que demonstra o parser em ação.

**Estrutura**:
1. Função auxiliar `test_expression(const char *expr)` que:
   - Tokeniza a expressão
   - Exibe tokens em formato legível
   - Exibe hexdump
   - Tenta conversão para RPN (stub)
   - Trata erros

2. Função `main()` que:
   - Executa testes com `LOCALE_POINT`
   - Executa testes com `LOCALE_COMMA`
   - Testa casos de erro

**Como estender**: Adicione novas chamadas a `test_expression()` com novos casos de teste.

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

### Fase 1: Tokenização ✅
- [x] Tokenização de operadores básicos (+, -, *, /, ^)
- [x] Tokenização de números (com suporte a locale)
- [x] Tokenização de funções (sin, cos, tan, abs, sqrt)
- [x] Tokenização de constantes (pi, e)
- [x] Tokenização de variáveis (x, theta, t)
- [x] Detecção de parênteses
- [x] Validação de variáveis (não mista)
- [x] Validação de sintaxe básica
- [x] Suporte a locale (ponto e vírgula decimal)
- [x] Debug output (hex, tokens)

### Fase 2: RPN ⏳
- [ ] Algoritmo Shunting Yard
- [ ] Suporte a precedência de operadores
- [ ] Suporte a associatividade
- [ ] Suporte a funções

### Fase 3: Avaliação ⏳
- [ ] Avaliador de RPN
- [ ] Substituição de variáveis
- [ ] Cálculo de constantes
- [ ] Tratamento de erros matemáticos

### Fase 4: Interface ⏳
- [ ] Plotagem de gráficos
- [ ] Interface CLI/GUI

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
Para adicionar nova função (ex: "log"):
1. Adicione `TOKEN_LOG = 155` em `tokens.h`
2. Adicione `CHECK_KEYWORD("log", TOKEN_LOG)` em `parser.c`
3. Adicione case no `debug_token_name()` em `debug.c`
4. Implemente suporte em RPN (Fase 2)
5. Implemente suporte em avaliador (Fase 3)

---

**Última atualização**: 2026-01-11
