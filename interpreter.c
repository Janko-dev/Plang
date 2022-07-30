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

Env* createEnv(Env* enclosing){
    Env* e = (Env*)malloc(sizeof(Env));
    if (e == NULL){
        plerror(-1, RUNTIME_ERROR, "Malloc failed at environment initialisation");
        return NULL;
    }
    e->map = (EnvMap**)malloc(sizeof(EnvMap*) * ENV_SIZE);
    if (e == NULL){
        plerror(-1, RUNTIME_ERROR, "Malloc failed at environment initialisation");
        return NULL;
    }
    memset(e->map, 0, sizeof(EnvMap*) * ENV_SIZE);
    e->enclosing = enclosing;
    return e;
}

void freeEnvMap(EnvMap** env){
    for (size_t i = 0; i < ENV_SIZE; i++){
        EnvMap* e = env[i];
        EnvMap* tmp;
        while(e != NULL){
            tmp = e;
            e = e->next;
            free(tmp->key);
            tmp->key = NULL;
            free(tmp);
            tmp = NULL;
        }
    }
    free(env);
    env = NULL;
}

void freeEnv(Env* env){
    freeEnvMap(env->map);
    env->map = NULL;
    free(env);
    env = NULL;
}

static EnvMap* lookup(EnvMap** env, char* s){
    EnvMap* e;
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

void define(Env* env, char* key, Obj* value){
    EnvMap* e;
    unsigned int hashval;
    if ((e = lookup(env->map, key)) == NULL){
        e = (EnvMap*)malloc(sizeof(*e));
        if (e == NULL || (e->key = strdup(key)) == NULL) return;
        hashval = hash(key);
        e->next = env->map[hashval];
        env->map[hashval] = e;
    }
    e->value = value;
}

void assign(Env* env, Token* name, Obj* value){
    EnvMap* e;
    if ((e = lookup(env->map, name->lexeme)) == NULL){
        if (env->enclosing != NULL){
            assign(env->enclosing, name, value);
            return;
        }
        plerror(name->line, RUNTIME_ERROR, "Undefined variable '%s'", name->lexeme);
        return;
    }

    e->value = value;
}

Obj* get(Env* env, Token* name){
    EnvMap* e;
    if ((e = lookup(env->map, name->lexeme)) == NULL){
        if (env->enclosing != NULL){
            return get(env->enclosing, name);
        }
        plerror(name->line, RUNTIME_ERROR, "Undefined variable '%s'", name->lexeme);
        return newObj(NIL_T);
    }
    return e->value;
}
#pragma endregion Environment

#pragma region Interpreter

static char* valueTypes[] = { "nil", "number", "string", "boolean" };

Obj* newObj(enum LiteralType type){
    Obj* obj = malloc(sizeof(Obj));
    if (obj == NULL) {
        plerror(-1, RUNTIME_ERROR, "Malloc failed at object constructor"); 
        return NULL;
    }
    obj->type = type;
    return obj;
}

Obj* newNum(double num){
    Obj* obj = newObj(NUM_T);
    obj->as.num = num;
    return obj;
}

Obj* newString(char* string){
    Obj* obj = newObj(STR_T);
    obj->as.string = string;
    return obj;
}

Obj* newBool(bool b){
    Obj* obj = newObj(BOOL_T);
    obj->as.boolean = b;
    return obj;
}

int isTruthy(Obj* obj){
    if (obj->type == NIL_T) return false;
    if (obj->type == BOOL_T) return obj->as.boolean;
    return true;
}

Obj* evaluate(Expr* expr, Env* env){
    switch (expr->type)
    {
    case BINARY: {
        if (expr->as.binary.op->type == AND){
            Obj* left = evaluate(expr->as.binary.left, env);
            if (!isTruthy(left)) return newBool(false);
            return evaluate(expr->as.binary.right, env);
        } else if (expr->as.binary.op->type == OR){
            Obj* left = evaluate(expr->as.binary.left, env);
            if (isTruthy(left)) return newBool(true);
            return evaluate(expr->as.binary.right, env);
        }
        
        Obj* left = evaluate(expr->as.binary.left, env);
        Obj* right = evaluate(expr->as.binary.right, env);

        switch (expr->as.binary.op->type)
        {
        case EQUAL_EQUAL: {
            if (left->type == NIL_T && right->type == NIL_T) return newBool(true);
            if (left->type == NIL_T) return newBool(false);
            return newBool(left->as.num == right->as.num);
        }
        case BANG_EQUAL: {
            if (left->type == NIL_T && right->type == NIL_T) return newBool(false);
            if (left->type == NIL_T) return newBool(true);
            return newBool(left->as.num != right->as.num);
        }
        case GREATER: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'greater than' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            return newBool(left->as.num > right->as.num);
        }
        case GREATER_EQUAL: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'greater than or equal to' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            return newBool(left->as.num >= right->as.num);
        }
        case LESS: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'less than' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            return newBool(left->as.num < right->as.num);
        }
        case LESS_EQUAL: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'less than or equal to' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            return newBool(left->as.num <= right->as.num);
        }
        case STAR: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'times' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            return newNum(left->as.num * right->as.num);
        }
        case SLASH: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'division' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            if (right->as.num == 0) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Division by zero error");
                return newObj(NIL_T);
            }
            return newNum(left->as.num / right->as.num);
        }
        case MINUS: {
            if (left->type != NUM_T || right->type != NUM_T) {
                plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'minus' operator is not defined for %s and %s", 
                    valueTypes[left->type], valueTypes[right->type]);
                return newObj(NIL_T);
            }
            return newNum(left->as.num - right->as.num);
        }
        case PLUS: {
            if (left->type == NUM_T && right->type == NUM_T){
                return newNum(left->as.num + right->as.num);
            }
            if (left->type == STR_T && right->type == STR_T){
                size_t len_left = strlen(left->as.string);
                size_t len_right = strlen(right->as.string);
                char* res = malloc(len_left + len_right);
                for (size_t i = 0; i < len_left; i++) res[i] = left->as.string[i];
                for (size_t i = len_left; i < len_left + len_right; i++) res[i] = right->as.string[i-len_left];
                res[len_left+len_right] = '\0';
                return newString(res);
            }
            plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Type mismatch, binary 'plus' operation is not defined for %s and %s", 
                valueTypes[left->type], valueTypes[right->type]);
            return newObj(NIL_T);
        }
        default:
            plerror(expr->as.binary.op->line, RUNTIME_ERROR, "Unreachable binary operator");
            return newObj(NIL_T);
        }
    } break;
    case TERNARY: {
        Obj* res = evaluate(expr->as.ternary.cond, env);
        if (isTruthy(res)) {
            return evaluate(expr->as.ternary.trueBranch, env);
        } else {
            return evaluate(expr->as.ternary.falseBranch, env);
        }
    } break;
    case UNARY: {
        Obj* right = evaluate(expr->as.unary.right, env);
        switch(expr->as.unary.op->type){
            case MINUS: {
                if (right->type != NUM_T) {
                    plerror(expr->as.unary.op->line, RUNTIME_ERROR, "Expected type '%s', but got '%s'", valueTypes[NUM_T], valueTypes[right->type]);
                    return newObj(NIL_T);
                }
                right->as.num = -right->as.num;
                return right;
            };
            case BANG: {
                right->as.boolean = !isTruthy(right);
                right->type = BOOL_T;
                return right;
            }
            default: 
                plerror(expr->as.unary.op->line, RUNTIME_ERROR, "Unreachable state");
                return newObj(NIL_T);
        }
    } break;
    case LITERAL: {
        switch (expr->as.literal.type){
        case NUM_T: return newNum(*(double*)expr->as.literal.value);
        case STR_T: return newString((char*)expr->as.literal.value);
        case BOOL_T: return newBool(expr->as.literal.value ? true : false);
        case NIL_T: return newObj(NIL_T);
        default: plerror(expr->as.unary.op->line, RUNTIME_ERROR, "Unreachable state");
            return newObj(NIL_T);
        }
    } break;
    case GROUPING: return evaluate(expr->as.group.expression, env); break;
    case VAREXPR: return get(env, expr->as.var.name); break;
    case ASSIGN: {
        Obj* val = evaluate(expr->as.assign.value, env);
        assign(env, expr->as.assign.name, val);
        return val;
    } break;
    default:
        plerror(-1, RUNTIME_ERROR, "Unreachable state");
        return newObj(NIL_T);
    }
}

void execute(Stmt* stmt, Env* env){
    switch (stmt->type)
    {
    case EXPR_STMT: evaluate(stmt->as.expr.expression, env); break;
    case PRINT_STMT: {
        Obj* val = evaluate(stmt->as.print.expression, env);
        switch (val->type)
        {
        case NUM_T:  printf("%f\n", val->as.num); break;
        case NIL_T:  printf("nil\n"); break;
        case BOOL_T: printf(val->as.boolean ? "true\n" : "false\n"); break;
        case STR_T:  printf("%s\n", val->as.string); break;
        default: break;
        }
    } break;
    case BLOCK_STMT: {
        Env* local = createEnv(env);
        for (size_t i = 0; i < stmt->as.block.list->index; i++){
            execute(&stmt->as.block.list->statements[i], local);
        }
        freeEnv(local);
    } break;
    case VAR_DECL_STMT:{
        if (stmt->as.var.initializer != NULL){
            Obj* init = evaluate(stmt->as.var.initializer, env);
            define(env, stmt->as.var.name->lexeme, init);
        } else define(env, stmt->as.var.name->lexeme, newObj(NIL_T)); 
    } break;
    case IF_STMT: {
        Obj* cond = evaluate(stmt->as.if_stmt.cond, env);
        if (isTruthy(cond)){
            execute(stmt->as.if_stmt.trueBranch, env);
        } else if (stmt->as.if_stmt.falseBranch != NULL){
            execute(stmt->as.if_stmt.falseBranch, env);
        }
    } break;
    case WHILE_STMT: {
        while (isTruthy(evaluate(stmt->as.while_stmt.cond, env))){
            execute(stmt->as.while_stmt.body, env);
        }
    } break;
    default: break;
    }
}

void interpret(StmtList* list, Env* env){
    for (size_t i = 0; i < list->index; i++){
        execute(&list->statements[i], env);
    }
}

#pragma endregion Environment