#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "evaluator.h"

/* Constantes matemáticas */
#define M_PI_CUSTOM 3.14159265358979323846
#define M_E_CUSTOM  2.71828182845904523536

/* Tamanho máximo da pilha de avaliação (otimização de desempenho) */
#define MAX_EVAL_STACK_SIZE 64

/* Retorna valor de uma constante */
static inline double get_constant_value(TokenType type) {
    switch (type) {
        case TOKEN_CONST_PI:
            return M_PI_CUSTOM;
        case TOKEN_CONST_E:
            return M_E_CUSTOM;
        default:
            return 0.0;
    }
}

/* Aplica função matemática (token função, argumento) */
static inline EvalResult apply_function(TokenType type, double arg) {
    EvalResult result = {EVAL_OK, 0.0};
    
    switch (type) {
        case TOKEN_SIN:
            result.value = sin(arg);
            break;
        case TOKEN_COS:
            result.value = cos(arg);
            break;
        case TOKEN_TAN:
            result.value = tan(arg);
            /* Verifica se resultou em infinito ou NaN */
            if (isnan(result.value) || isinf(result.value)) {
                result.error = EVAL_MATH_ERROR;
                return result;
            }
            break;
        case TOKEN_ABS:
            result.value = fabs(arg);
            break;
        case TOKEN_SQRT:
            if (arg < 0.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = sqrt(arg);
            break;
        case TOKEN_EXP:
            result.value = exp(arg);
            break;
        case TOKEN_LOG:
            if (arg <= 0.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = log(arg);
            break;
        case TOKEN_LOG10:
            if (arg <= 0.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = log10(arg);
            break;
        case TOKEN_SINH:
            result.value = sinh(arg);
            break;
        case TOKEN_COSH:
            result.value = cosh(arg);
            break;
        case TOKEN_TANH:
            result.value = tanh(arg);
            break;
        case TOKEN_ASIN:
            if (arg < -1.0 || arg > 1.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = asin(arg);
            break;
        case TOKEN_ACOS:
            if (arg < -1.0 || arg > 1.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = acos(arg);
            break;
        case TOKEN_ATAN:
            result.value = atan(arg);
            break;
        case TOKEN_ASINH:
            result.value = asinh(arg);
            break;
        case TOKEN_ACOSH:
            if (arg < 1.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = acosh(arg);
            break;
        case TOKEN_ATANH:
            if (arg <= -1.0 || arg >= 1.0) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            result.value = atanh(arg);
            break;
        case TOKEN_CEIL:
            result.value = ceil(arg);
            break;
        case TOKEN_FLOOR:
            result.value = floor(arg);
            break;
        case TOKEN_FRAC:
            /* Parte fracionária: frac(x) = x - floor(x) */
            result.value = arg - floor(arg);
            break;
        default:
            result.error = EVAL_MATH_ERROR;
            break;
    }
    
    /* Verifica se resultado é válido */
    if (isnan(result.value) || isinf(result.value)) {
        result.error = EVAL_MATH_ERROR;
    }
    
    return result;
}

/* Aplica operador binário (token operador, operando esquerdo, operando direito) */
static inline EvalResult apply_operator(TokenType type, double left, double right) {
    EvalResult result = {EVAL_OK, 0.0};
    
    switch (type) {
        case TOKEN_PLUS:
            result.value = left + right;
            break;
        case TOKEN_MINUS:
            result.value = left - right;
            break;
        case TOKEN_MULT:
            result.value = left * right;
            break;
        case TOKEN_DIV:
            /* Erro específico para divisão por zero */
            if (right == 0.0) {
                result.error = EVAL_DIVISION_BY_ZERO;
                return result;
            }
            result.value = left / right;
            break;
        case TOKEN_POW:
            result.value = pow(left, right);
            /* pow pode retornar NaN para casos como (-1)^0.5 */
            if (isnan(result.value)) {
                result.error = EVAL_DOMAIN_ERROR;
                return result;
            }
            break;
        default:
            result.error = EVAL_MATH_ERROR;
            break;
    }
    
    /* Verifica overflow/underflow */
    if (isnan(result.value) || isinf(result.value)) {
        result.error = EVAL_MATH_ERROR;
    }
    
    return result;
}

/* Verifica se token é operador binário */
static int is_binary_operator(TokenType type) {
    return (type == TOKEN_PLUS || type == TOKEN_MINUS || 
            type == TOKEN_MULT || type == TOKEN_DIV || type == TOKEN_POW);
}

/* Verifica se token é operador unário (ex: NEG) */
static int is_unary_operator(TokenType type) {
    return (type == TOKEN_NEG);
}

/* Verifica se token é função unária */
static int is_unary_function(TokenType type) {
    return (type >= TOKEN_FUNCTION_START && type <= TOKEN_FUNCTION_END);
}

/* Verifica se token é variável */
static int is_variable(TokenType type) {
    return (type >= TOKEN_VARIABLE_START && type <= TOKEN_VARIABLE_END);
}

/* Verifica se token é constante */
static int is_constant(TokenType type) {
    return (type >= TOKEN_CONST_START && type <= TOKEN_CONST_END);
}

/* Avalia expressão em RPN */
EvalResult evaluator_eval_rpn(const TokenBuffer *rpn, double var_value) {
    EvalResult result = {EVAL_OK, 0.0};
    
    if (!rpn || !rpn->tokens || rpn->size == 0) {
        result.error = EVAL_STACK_ERROR;
        return result;
    }
    
    /* Pilha estática de valores (sem malloc/free para otimização) */
    double stack[MAX_EVAL_STACK_SIZE];
    int stack_top = -1;
    
    /* Processa cada token da expressão RPN usando switch para reduzir branches */
    for (int i = 0; i < rpn->size; i++) {
        Token token = rpn->tokens[i];
        TokenType type = token.type;

        switch (type) {
            case TOKEN_END:
                goto evaluation_end;

            case TOKEN_NUMBER:
                if (stack_top >= MAX_EVAL_STACK_SIZE - 1) {
                    result.error = EVAL_STACK_ERROR;
                    return result;
                }
                stack[++stack_top] = rpn->values[token.value_index];
                break;

            case TOKEN_VARIABLE_X:
            case TOKEN_VARIABLE_THETA:
            case TOKEN_VARIABLE_T:
                if (stack_top >= MAX_EVAL_STACK_SIZE - 1) {
                    result.error = EVAL_STACK_ERROR;
                    return result;
                }
                stack[++stack_top] = var_value;
                break;

            case TOKEN_CONST_PI:
            case TOKEN_CONST_E:
                if (stack_top >= MAX_EVAL_STACK_SIZE - 1) {
                    result.error = EVAL_STACK_ERROR;
                    return result;
                }
                stack[++stack_top] = get_constant_value(type);
                break;

            /* Operadores binários */
            case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_MULT:
            case TOKEN_DIV:
            case TOKEN_POW:
                if (stack_top < 1) {
                    result.error = EVAL_STACK_ERROR;
                    return result;
                }
                {
                    double right = stack[stack_top--];
                    double left = stack[stack_top--];
                    EvalResult op_result = apply_operator(type, left, right);
                    if (op_result.error != EVAL_OK) return op_result;
                    stack[++stack_top] = op_result.value;
                }
                break;

            /* Operador unário (NEG) */
            case TOKEN_NEG:
                if (stack_top < 0) {
                    result.error = EVAL_STACK_ERROR;
                    return result;
                }
                {
                    double arg = stack[stack_top--];
                    stack[++stack_top] = -arg;
                }
                break;

            /* Funções unárias (lista explícita) */
            case TOKEN_SIN: case TOKEN_COS: case TOKEN_TAN: case TOKEN_ABS:
            case TOKEN_SQRT: case TOKEN_EXP: case TOKEN_LOG: case TOKEN_LOG10:
            case TOKEN_SINH: case TOKEN_COSH: case TOKEN_TANH:
            case TOKEN_ASIN: case TOKEN_ACOS: case TOKEN_ATAN:
            case TOKEN_ASINH: case TOKEN_ACOSH: case TOKEN_ATANH:
            case TOKEN_CEIL: case TOKEN_FLOOR: case TOKEN_FRAC:
                if (stack_top < 0) {
                    result.error = EVAL_STACK_ERROR;
                    return result;
                }
                {
                    double arg = stack[stack_top--];
                    EvalResult func_result = apply_function(type, arg);
                    if (func_result.error != EVAL_OK) return func_result;
                    stack[++stack_top] = func_result.value;
                }
                break;

            default:
                /* Token desconhecido */
                result.error = EVAL_MATH_ERROR;
                return result;
        }
    }

evaluation_end: ;
    
    /* Deve sobrar exatamente 1 valor na pilha */
    if (stack_top != 0) {
        result.error = EVAL_STACK_ERROR;
        return result;
    }
    
    result.value = stack[0];
    
    return result;
}
