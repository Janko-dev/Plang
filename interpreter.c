#include "interpreter.h"
#define UTILS_IMPLEMENT
#include "utils.h"

#pragma region Environment
static unsigned int hash(char* s){
    unsigned int hashval;
    for (hashval = 0; *s != '\0'; s++){
        hashval = *s + 31 * hashval;
    }

    return hashval % HASH_SIZE;
}

void freeEnv(Env* env[ENV_SIZE]){
    for (size_t i = 0; i < ENV_SIZE; i++){
        Env* e = env[i];
        Env* tmp;
        while(e != NULL){
            tmp = e;
            e = e->next;
            free(tmp->key);
            free(tmp);
        }
    }
}

static Env* lookup(Env* env[ENV_SIZE], char* s){
    Env* e;
    for (e = env[hash(s)]; e != NULL; e = e->next){
        if (strcmp(s, e->key) == 0){
            return e;
        }
    }
    return NULL;
}

static char* strdup(char* s){
    char* p = (char*)malloc(strlen(s) + 1);
    if (p != NULL) strcpy(p, s);
    return p;
}

void define(Env* env[ENV_SIZE], char* key, LiteralExpr value){
    Env* e;
    unsigned int hashval;
    if ((e = lookup(env, key)) == NULL){
        e = (Env*)malloc(sizeof(*e));
        if (e == NULL || (e->key = strdup(key)) == NULL) return;
        hashval = hash(key);
        e->next = env[hashval];
        env[hashval] = e;
    }
    e->value = value;
}

void assign(Env* env[ENV_SIZE], Token* name, LiteralExpr value){
    Env* e;
    if ((e = lookup(env, name->lexeme)) == NULL){
        plerror(name->line, RUNTIME_ERROR, "Undefined variable '%s'", name->lexeme);
        return;
    }
    e->value = value;
}

LiteralExpr get(Env* env[ENV_SIZE], Token* name){
    Env* e;
    if ((e = lookup(env, name->lexeme)) == NULL){
        plerror(name->line, RUNTIME_ERROR, "Undefined variable '%s'", name->lexeme);
        return (LiteralExpr){};
    }
    return e->value;
}
#pragma endregion Environment

#pragma region Interpreter

static char* valueTypes[] = { "nil", "number", "string", "boolean" };

int isTruthy(LiteralExpr obj){
    if (obj.value == NULL) return false;
    if (obj.type == BOOL_T) return *(int*)obj.value;
    return true;
}

LiteralExpr evaluate(Expr* expr, Env* env[ENV_SIZE]){
    switch (expr->type)
    {
    case BINARY: {
        LiteralExpr left = evaluate(expr->as.binary.left, env);
        LiteralExpr right = evaluate(expr->as.binary.right, env);

        switch (expr->as.binary.op->type)
        {
        case EQUAL_EQUAL: {
            if (left.value == NULL && right.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)true};
            if (left.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)false};
            int res = *(double*)left.value == *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)&res}; 
        }
        case BANG_EQUAL: {
            if (left.value == NULL && right.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)false};
            if (left.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)true};
            int res = *(double*)left.value != *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)&res};
        }
        case GREATER: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'greater than' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            int res = *(double*)left.value > *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)&res}; 
        }
        case GREATER_EQUAL: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'greater than or equal to' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            int res = *(double*)left.value >= *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)&res}; 
        }
        case LESS: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'less than' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            int res = *(double*)left.value < *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)&res}; 
        }
        case LESS_EQUAL: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'less than or equal to' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            int res = *(double*)left.value <= *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)&res};
        }
        case STAR: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'times' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            double res = *(double*)left.value * *(double*)right.value;
            return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
        }
        case SLASH: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'division' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            double res = *(double*)left.value / *(double*)right.value;
            return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
        }
        case MINUS: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'minus' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return (LiteralExpr){};
            }
            double res = *(double*)left.value - *(double*)right.value;
            return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
        }
        case PLUS: {
            if (left.type == NUM_T && right.type == NUM_T){
                double res = *(double*)left.value + *(double*)right.value;
                return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
            }
            if (left.type == STR_T && right.type == STR_T){
                size_t len_left = strlen((char*)left.value);
                size_t len_right = strlen((char*)right.value);
                char* res = malloc(len_left + len_right); // double '\0' so -1
                for (size_t i = 0; i < len_left; i++) res[i] = ((char*)left.value)[i];
                for (size_t i = len_left; i < len_left + len_right; i++) res[i] = ((char*)right.value)[i-len_left];
                res[len_left+len_right] = '\0';
                return (LiteralExpr){.type=STR_T, .value=(void*)res};
            }
            plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'plus' operation is not defined for %s and %s", 
                valueTypes[left.type], valueTypes[right.type]);
            return (LiteralExpr){};
        }
        default:
            plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Unreachable binary operator");
            return (LiteralExpr){};
        }
    } break;
    case TERNARY: {
        LiteralExpr res = evaluate(expr->as.ternary.cond, env);
        // if (res.type != BOOL_T){
        //     plerror(-1, RUNTIME_ERROR, "Expected type '%s', but got type '%s'", valueTypes[BOOL_T], valueTypes[res.type]);
        //     return (LiteralExpr){};
        // }
        if (isTruthy(res)) {
            return evaluate(expr->as.ternary.trueBranch, env);
        } else {
            return evaluate(expr->as.ternary.falseBranch, env);
        }
    } break;
    case UNARY: {
        LiteralExpr right = evaluate(expr->as.unary.right, env);
        switch(expr->as.unary.op->type){
            case MINUS: {
                if (right.type != NUM_T) {
                    plerror(expr->as.unary.op->line, RUNTIME_ERROR, "Expected type '%s', but got '%s'", valueTypes[NUM_T], valueTypes[right.type]);
                    return (LiteralExpr){};
                }
                *(double*)right.value *= -1;
                return right;
            };
            case BANG: {
                int res = !isTruthy(right);
                return (LiteralExpr){.type=BOOL_T, .value=(void*)&res};
            }
            default: 
                plerror(expr->as.unary.op->line, RUNTIME_ERROR, "Unreachable state");
                return (LiteralExpr){};
        }
    } break;
    case LITERAL: return expr->as.literal; break;
    case GROUPING: return evaluate(expr->as.group.expression, env); break;
    case VAREXPR: return get(env, expr->as.var.name); break;
    case ASSIGN: {
        LiteralExpr value = evaluate(expr->as.assign.value, env);
        assign(env, expr->as.assign.name, value);
        return value;
    } break;
    default:
        plerror(-1, RUNTIME_ERROR, "Unreachable state");
        return (LiteralExpr){};
    }
}

void execute(Stmt* stmt, Env* env[ENV_SIZE]){
    switch (stmt->type)
    {
    case EXPR_STMT: evaluate(stmt->as.expr.expression, env); break;
    case PRINT_STMT: {
        LiteralExpr val = evaluate(stmt->as.print.expression, env);
        switch (val.type)
        {
        case NUM_T:  printf("%f\n", *(double*)val.value); break;
        case NIL_T:  printf("nil\n", val.value); break;
        case BOOL_T: printf(val.value ? "true" : "false"); break;
        case STR_T:  printf("%s\n", (char*)val.value); break;
        default: break;
        }
    } break;
    case VAR_DECL_STMT:{
        if (stmt->as.var.initializer != NULL){
            LiteralExpr init = evaluate(stmt->as.var.initializer, env);
            define(env, stmt->as.var.name->lexeme, init);
        } else define(env, stmt->as.var.name->lexeme, (LiteralExpr){}); 
    } break;
    }
}

void interpret(StmtList* list, Env* env[ENV_SIZE]){
    for (size_t i = 0; i < list->index; i++){
        execute(&list->statements[i], env);
    }
}

#pragma endregion Environment