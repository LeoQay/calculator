/*
 * Simple Calc Language
 *
 *
 * <1L sign>    = <+>, <-> // unary
 * <2L sign>    = <*>, </> // binary
 * <3L sign>    = <+>, <-> // binary
 *
 * <expression> = <term> [<3L sign> <term>]
 *
 * <term> = <term spec> [<term cont>]
 * <term> = <mul> [<term cont>]
 *
 * <term cont> = <2L sign> <mul>
 * <term cont> = <term spec>
 *
 * <term spec> = (<expression>)<val>
 * <term spec> = (<expression>)<variable>
 * <term spec> = (<expression>)
 *
 * <mul> = <1L sign> <mul>
 * <mul> = (<expression>)
 * <mul> = <val>
 * <mul> = <variable>
 *
 * <val> = <int>
 * <val> = <float>
 *
 * <float> = <int>.[<int>]
 *
 * <int> = <num>[<num>]
 *
 * <num> = {0,1,2,3,4,5,6,7,8,9}
 *
 * <variable> = <start correct sym> [<correct sym>]
 *
 * <start correct sym> = <word> or '_'
 * <correct sym> = <word> or <num> or '_'
 * <word> = {a .. z, A .. Z}
 */

#include "var_mgr.h"

#define comp 0.0000000001

typedef unsigned long ulong;
typedef long long int_type;
typedef double float_type;

typedef enum {
    INVALID_TOKEN = 1,
    MUL_UNEXPECT = 2,
    MISS_CLOSE_PARENT = 3,
    TERM_SPEC_ERR = 4,
    EMPTY_MUL = 5,
    EXPR_UNEXPECT = 6,
    VAR_NAME_ERROR = 7,
    NUMBER_TOKEN_ERROR = 8,
    MISS_OPEN_BRACKET = 9,
    EXEC_STACK_ERR = 10,
    ZERO_DIVIDE = 11,
    ERR_ERANGE = 12
} Exception;


typedef enum {
    WRONG,
    PLUS,
    MINUS,
    MUL,
    DIV,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    VAR,
    INT,
    FLOAT,
    END
} TokenType;


typedef enum {
    ALLOW_EXPR,
    SIMPLE
} InputVariable;


enum { MAX_TOKEN_LEN = 10000,
        MAX_NAME_LEN = 6
};


typedef struct {
    Stack *stack;
    FILE *in_stream;
    FILE *out_stream;
    FILE *err_stream;
    int was_used;
    char cur_str[MAX_TOKEN_LEN + 1];
    TokenType cur_t;
    Elem *result;
    VarSet *vars;
    InputVariable inp_t;
    Stack *buffer;
    int is_stack_good;
} Calc;


typedef int (*comp_t) (const void *, const void *);


void error(Calc *calc, int ret_code);

Calc* init_calc(void);
void delete_calc(Calc *calc);

void set_in_stream(Calc *calc, FILE *new_stream);
void set_out_stream(Calc *calc, FILE *new_stream);
void set_err_stream(Calc *calc, FILE *new_stream);

int up_used(Calc *calc);
int down_used(Calc *calc);

int ejudge_process_str(Calc *calc);
void input_mgr (Calc *calc);
int process_str(Calc *calc);
int build_postfix(Calc *calc);

int expression (Calc *calc);
int term (Calc *calc);
int term_spec(Calc *calc);
int mul (Calc *calc);
int get_token (Calc *calc);

void get_num(FILE *stream, char* dest, int max_len, int *what);
void get_var (FILE *stream, char* dest, int max_len, int *what);

Elem *replace_var(Calc *calc, Elem *elem);
int process_new_vars(Calc *calc, InputVariable type);
int exec_stack(Calc *calc);

void print_dict(Calc *calc, FILE *stream);
void print_var_set(VarSet *set, FILE *stream);
void print_stack(Stack *stack, FILE *stream);
void print_elem(Elem *elem, FILE *stream);

Elem* copy_elem(Elem *elem);
Stack* copy_stack(Stack *stack);

Elem* elem_str(StackType type, char *str);
Elem* elem_int(int_type val);
Elem* elem_float(float_type val);

long miss_line(Calc *calc);
void put_str_back(Calc *calc, char *str);
Elem* check_var (Calc *calc);
int check_str_input(Calc *calc, char *src);

int comp_elem_str(Elem **a, Elem **b);
