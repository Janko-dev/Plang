#include "interpreter.h"
#define UTILS_IMPLEMENT
#include "utils.h"

static char* source;

static LiteralExpr nil_obj();
static LiteralExpr num_obj(double num);
static LiteralExpr string_obj(char* string);
static LiteralExpr bool_obj(bool b);

#pragma region Environment
static unsigned int hash(char* s){
    unsigned int hashval;
    for (hashval = 0; *s != '\0'; s++){
        hashval = *s + 31 * hashval;
    }

    return hashval % ENV_SIZE;
}

Env* create_env(Env* enclosing){
    Env* e = (Env*)malloc(sizeof(Env));
    if (e == NULL){
        plerror(-1, -1, MEMORY_ERR, "Malloc failed at environment initialisation");
        exit(1);
    }
    e->map = (EnvMap**)malloc(sizeof(EnvMap*) * ENV_SIZE);
    if (e == NULL){
        plerror(-1, -1, MEMORY_ERR, "Malloc failed at environment initialisation");
        exit(1);
    }
    memset(e->map, 0, sizeof(EnvMap*) * ENV_SIZE);
    e->enclosing = enclosing;
    return e;
}

void free_env_map(EnvMap** env){
    for (size_t i = 0; i < ENV_SIZE; i++){
        EnvMap* e = env[i];
        EnvMap* tmp;
        while(e != NULL){
            tmp = e;
            e = e->next;
            free(tmp->key);
            free(tmp);
        }
    }
    free(env);
    env = NULL;
}

void free_env(Env* env){
    free_env_map(env->map);
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

void define(Env* env, char* key, LiteralExpr value){
    EnvMap* e;
    if ((e = lookup(env->map, key)) == NULL){
        e = (EnvMap*)malloc(sizeof(*e));
        if (e == NULL) {
            plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for Environment node: %s", key);
            exit(1);
        }
        e->key = malloc(sizeof(char) * strlen(key) + 1);
        if (e->key == NULL) {
            plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for Environment node key");
            exit(1);
        } 
        strcpy(e->key, key);
        unsigned int hashval = hash(key);
        e->next = env->map[hashval];
        env->map[hashval] = e;
    }
    e->value = value;
}

static char* get_lexeme(Token* tok){
    size_t n = tok->count - tok->start;
    char* buf = (char*)malloc(n * sizeof(char) + 1);
    for (size_t i = 0; i < n; i++) buf[i] = source[tok->start+i];
    buf[n] = '\0';
    return buf; 
}

void assign(Env* env, Token* name, LiteralExpr value){
    EnvMap* e;
    char* lexeme = get_lexeme(name);
    if ((e = lookup(env->map, lexeme)) == NULL){
        if (env->enclosing != NULL){
            free(lexeme);
            assign(env->enclosing, name, value);
            return;
        }
        plerror(name->line, get_column(name), RUNTIME_ERR, "Undefined variable '%s'", lexeme);
        free(lexeme);
        return;
    }
    e->value = value;
    free(lexeme);
}

LiteralExpr get(Env* env, Token* name){
    EnvMap* e;
    char* lexeme = get_lexeme(name);
    if ((e = lookup(env->map, lexeme)) == NULL){
        if (env->enclosing != NULL){
            free(lexeme);
            return get(env->enclosing, name);
        }
        plerror(name->line, get_column(name), RUNTIME_ERR, "Undefined variable '%s'", lexeme);
        free(lexeme);
        return nil_obj();
    }
    return e->value;
}
#pragma endregion Environment

#pragma region Interpreter

static char* valueTypes[] = { "nil", "number", "string", "boolean" };

LiteralExpr nil_obj(){
    return (LiteralExpr){
        .type = NIL_T
    };
}

LiteralExpr num_obj(double num){
    return (LiteralExpr){
        .type = NUM_T,
        .as.number = num
    };
}

LiteralExpr string_obj(char* string){
    return (LiteralExpr){
        .type = STR_T,
        .as.string = string
    };
}

LiteralExpr bool_obj(bool b){
    return (LiteralExpr){
        .type = BOOL_T,
        .as.boolean = b
    };
}

int isTruthy(LiteralExpr obj){
    if (obj.type == NIL_T) return false;
    if (obj.type == BOOL_T) return obj.as.boolean;
    return true;
}

LiteralExpr evaluate(Expr* expr, Env* env){
    switch (expr->type)
    {
    case BINARY: {
        if (expr->as.binary.op->type == AND){
            LiteralExpr left = evaluate(expr->as.binary.left, env);
            if (!isTruthy(left)) return bool_obj(false);
            return evaluate(expr->as.binary.right, env);
        } else if (expr->as.binary.op->type == OR){
            LiteralExpr left = evaluate(expr->as.binary.left, env);
            if (isTruthy(left)) return bool_obj(true);
            return evaluate(expr->as.binary.right, env);
        }
        
        LiteralExpr left = evaluate(expr->as.binary.left, env);
        LiteralExpr right = evaluate(expr->as.binary.right, env);

        switch (expr->as.binary.op->type)
        {
        case EQUAL_EQUAL: {
            if (left.type == NIL_T && right.type == NIL_T) return bool_obj(true);
            if (left.type == NIL_T) return bool_obj(false);
            return bool_obj(left.as.number == right.as.number);
        }
        case BANG_EQUAL: {
            if (left.type == NIL_T && right.type == NIL_T) return bool_obj(false);
            if (left.type == NIL_T) return bool_obj(true);
            return bool_obj(left.as.number != right.as.number);
        }
        case GREATER: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'greater than' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            return bool_obj(left.as.number > right.as.number);
        }
        case GREATER_EQUAL: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'greater than or equal to' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            return bool_obj(left.as.number >= right.as.number);
        }
        case LESS: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'less than' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            return bool_obj(left.as.number < right.as.number);
        }
        case LESS_EQUAL: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'less than or equal to' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            return bool_obj(left.as.number <= right.as.number);
        }
        case STAR: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'times' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            return num_obj(left.as.number * right.as.number);
        }
        case SLASH: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'division' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            if (right.as.number == 0) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Division by zero error");
                return nil_obj();
            }
            return num_obj(left.as.number / right.as.number);
        }
        case MINUS: {
            if (left.type != NUM_T || right.type != NUM_T) {
                plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'minus' operator is not defined for %s and %s", 
                    valueTypes[left.type], valueTypes[right.type]);
                return nil_obj();
            }
            return num_obj(left.as.number - right.as.number);
        }
        case PLUS: {
            if (left.type == NUM_T && right.type == NUM_T){
                return num_obj(left.as.number + right.as.number);
            }
            if (left.type == STR_T && right.type == STR_T){
                size_t len_left = strlen(left.as.string);
                size_t len_right = strlen(right.as.string);
                char* res = malloc(len_left + len_right + 1);
                for (size_t i = 0; i < len_left; i++) res[i] = left.as.string[i];
                for (size_t i = len_left; i < len_left + len_right; i++) res[i] = right.as.string[i-len_left];
                res[len_left+len_right] = '\0';
                return string_obj(res);
            }
            plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Type mismatch, binary 'plus' operation is not defined for %s and %s", 
                valueTypes[left.type], valueTypes[right.type]);
            return nil_obj();
        }
        default:
            plerror(expr->as.binary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Unreachable binary operator");
            return nil_obj();
        }
    } break;
    case TERNARY: {
        LiteralExpr res = evaluate(expr->as.ternary.cond, env);
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
                    plerror(expr->as.unary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Expected type '%s', but got '%s'", valueTypes[NUM_T], valueTypes[right.type]);
                    return nil_obj();
                }
                right.as.number = -right.as.number;
                return right;
            };
            case BANG: {
                right.as.boolean = !isTruthy(right);
                right.type = BOOL_T;
                return right;
            }
            default: 
                plerror(expr->as.unary.op->line, get_column(expr->as.binary.op), RUNTIME_ERR, "Unreachable state");
                return nil_obj();
        }
    } break;
    case LITERAL: {
        return expr->as.literal;
    } break;
    case GROUPING: return evaluate(expr->as.group.expression, env); break;
    case VAREXPR: return get(env, expr->as.var.name); break;
    case ASSIGN: {
        LiteralExpr val = evaluate(expr->as.assign.value, env);
        assign(env, expr->as.assign.name, val);
        return val;
    } break;
    default:
        plerror(-1, -1, RUNTIME_ERR, "Unreachable state");
        return nil_obj();
    }
}

void execute(Stmt stmt, Env* env){
    switch (stmt.type)
    {
    case EXPR_STMT: evaluate(stmt.as.expr.expression, env); break;
    case PRINT_STMT: {
        LiteralExpr val = evaluate(stmt.as.print.expression, env);
        switch (val.type)
        {
        case NUM_T:  printf("%f\n", val.as.number); break;
        case NIL_T:  printf("nil\n"); break;
        case BOOL_T: printf(val.as.boolean ? "true\n" : "false\n"); break;
        case STR_T:  printf("%s\n", val.as.string); break;
        default: break;
        }
    } break;
    case BLOCK_STMT: {
        Env* local = create_env(env);
        for (size_t i = 0; i < stmt.as.block.list->index; i++){
            execute(stmt.as.block.list->statements[i], local);
        }
        free_env(local);
    } break;
    case VAR_DECL_STMT:{
        char* lexeme = get_lexeme(stmt.as.var.name);
        if (stmt.as.var.initializer != NULL){
            LiteralExpr init = evaluate(stmt.as.var.initializer, env);
            define(env, lexeme, init);
        } else define(env, lexeme, nil_obj());
        free(lexeme);
    } break;
    case IF_STMT: {
        LiteralExpr cond = evaluate(stmt.as.if_stmt.cond, env);
        if (isTruthy(cond)){
            execute(*stmt.as.if_stmt.trueBranch, env);
        } else if (stmt.as.if_stmt.falseBranch != NULL){
            execute(*stmt.as.if_stmt.falseBranch, env);
        }
    } break;
    case WHILE_STMT: {
        while (isTruthy(evaluate(stmt.as.while_stmt.cond, env))){
            execute(*stmt.as.while_stmt.body, env);
        }
    } break;
    default: break;
    }
}

void interpret(StmtList* list, Env* env, char* code_source){
    source = code_source;
    for (size_t i = 0; i < list->index; i++){
        execute(list->statements[i], env);
    }
}

#pragma endregion Environment