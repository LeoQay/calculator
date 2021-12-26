#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "calc.h"

Calc* init_calc (void)
{
    Calc *calc = malloc(sizeof (Calc));
    calc->cur_t = END;
    calc->was_used = 1;
    calc->in_stream = stdin;
    calc->out_stream = stdout;
    calc->err_stream = stdout;
    calc->stack = init_stack();
    calc->result = NULL;
    calc->buffer = NULL;
    calc->vars = init_var_set();
    calc->is_stack_good = 0;

    return calc;
}


void delete_calc(Calc *calc)
{
    delete_stack(calc->buffer);

    delete_stack(calc->stack);

    if (calc->result != NULL)
    {
        free(calc->result);
    }

    delete_var_set(calc->vars);

    free(calc);
}


void input_mgr (Calc *calc)
{
    int is_end = 0;

    do {
        if (-1 == process_str(calc))
        {
            fprintf(calc->out_stream, "Finish state:\n");
            print_dict(calc, calc->out_stream);
            is_end = 1;
        }
    } while (!is_end);
}


int ejudge_process_str(Calc *calc)
{
    int ret;

    if (0 != (ret = build_postfix(calc)))
    {
        return 1;
    }

    if (0 != (ret = process_new_vars(calc, SIMPLE)))
    {
        return 2;
    }

    if (0 != (ret = exec_stack(calc)))
    {
        return 3;
    }

    if (calc->result->type == INT_NUM)
    {
        fprintf(calc->out_stream, "%lld\n", *(int_type*)&calc->result->data);
    }
    else if (calc->result->type == FLOAT_NUM)
    {
        fprintf(calc->out_stream, "%f\n", *(float_type*)&calc->result->data);
    }
    else
    {
        return 3;
    }

    return 0;
}


int process_str (Calc *calc)
{
    int ret;

    Elem *var_name;

    fprintf(calc->out_stream, ">>>");

    if (0 == check_str_input(calc, "@end"))
    {
        return -1;
    }

    if (0 == check_str_input(calc, "@dict"))
    {
        print_dict(calc, calc->out_stream);
        miss_line(calc);
        return 0;
    }

    if (0 == check_str_input(calc, "@save"))
    {
        miss_line(calc);

        if (calc->is_stack_good == 1)
        {
            delete_stack(calc->buffer);
            calc->buffer = copy_stack(calc->stack);
        }
        else
            fprintf(calc->out_stream, "Stack is bad\n");

        return 0;
    }

    if (0 == check_str_input(calc, "@use"))
    {
        miss_line(calc);

        if (calc->buffer == NULL)
        {
            fprintf(calc->out_stream, "Buffer is empty\n");
            return 0;
        }

        calc->is_stack_good = 1;

        delete_stack(calc->stack);
        calc->stack = copy_stack(calc->buffer);

        if (0 != (ret = exec_stack(calc)))
        {
            error(calc, ret);
        }

        if (calc->result->type == ERROR)
            print_elem(calc->result, calc->err_stream);
        else
            print_elem(calc->result, calc->out_stream);
        return 0;
    }

    if (NULL != (var_name = check_var(calc)))
    {
        if (0 != (ret = build_postfix(calc)))
        {
            calc->is_stack_good = 0;
            error(calc, ret);
        }
        else calc->is_stack_good = 1;

        if (ret == 0 && 0 != (ret = process_new_vars(calc, ALLOW_EXPR)))
        {
            error(calc, ret);
        }

        if (ret == 0 && 0 != (ret = exec_stack(calc)))
        {
            error(calc, ret);
        }

        if (calc->result->type != INT_NUM && calc->result->type != FLOAT_NUM)
        {
            fprintf(calc->err_stream, "I get not value\n");
            print_elem(calc->result, calc->err_stream);
        }
        else
        {
            add_var(calc->vars, var_name, calc->result);
            calc->result = NULL;
        }
    }
    else
    {
        if (0 != (ret = build_postfix(calc)))
        {
            calc->is_stack_good = 0;
            error(calc, ret);
        }
        else calc->is_stack_good = 1;

        if (ret == 0 && 0 != (ret = process_new_vars(calc, ALLOW_EXPR)))
        {
            error(calc, ret);
        }

        if (ret == 0 && 0 != (ret = exec_stack(calc)))
        {
            error(calc, ret);
        }

        if (calc->result->type == ERROR)
            print_elem(calc->result, calc->err_stream);
        else
            print_elem(calc->result, calc->out_stream);
    }

    return 0;
}


int build_postfix(Calc *calc)
{
    clear(calc->stack);

    int ret = expression(calc);

    up_used(calc);

    if (ret != 0 || calc->cur_t != END)
    {
        if (calc->cur_t != END) miss_line(calc);

        if (calc->result != NULL) free(calc->result);

        if (ret == 0 && calc->cur_t == CLOSE_BRACKET) ret = MISS_OPEN_BRACKET;

        error(calc, ret);

        clear(calc->stack);
        return 1;
    }

    return 0;
}


void error(Calc *calc, int ret_code)
{
    switch (ret_code)
    {
        case ZERO_DIVIDE:
            calc->result = elem_str(ERROR, "Zero dividing");
            break;
        case MISS_OPEN_BRACKET:
            calc->result = elem_str(ERROR, "Miss open bracket");
            break;
        case EXPR_UNEXPECT:
        case INVALID_TOKEN:
            calc->result = elem_str(ERROR, "Invalid token");
            break;
        case EMPTY_MUL:
            calc->result = elem_str(ERROR, "Missing operand");
            break;
        case VAR_NAME_ERROR:
            calc->result = elem_str(ERROR, "Wrong variable token");
            break;
        case NUMBER_TOKEN_ERROR:
            calc->result = elem_str(ERROR, "Wrong number token");
            break;
        case 0:
        case MISS_CLOSE_PARENT:
            calc->result = elem_str(ERROR, "Miss close bracket");
            break;
        case EXEC_STACK_ERR:
        case MUL_UNEXPECT:
        case TERM_SPEC_ERR:
            calc->result = elem_str(ERROR, "How you do it");
            break;
        default:
            calc->result = elem_str(ERROR, "Error!");
            break;
    }
}


int expression (Calc *calc)
{
    int ret;

    if (0 != (ret = term(calc))) { return ret; }

    while (1)
    {
        if (0 != (ret = get_token(calc))) { return ret; }

        switch (calc->cur_t)
        {
            case CLOSE_BRACKET:
            case END:
                return 0;
            case PLUS:
            case MINUS:
            {
                Elem *elem = elem_str(BIN_OP, calc->cur_str);

                up_used(calc);

                if (0 != (ret = term(calc))) { free(elem); return ret; }

                push(calc->stack, elem);

                break;
            }
            default:
                up_used(calc);
                return EXPR_UNEXPECT;
        }
    }
}


int term_spec (Calc *calc)
{
    int ret;

    if (0 != (ret = get_token(calc))) { return ret; }

    if (calc->cur_t != OPEN_BRACKET) { return TERM_SPEC_ERR; }

    if (0 != (ret = mul(calc))) { return ret; }

    Elem *elem = elem_str(BIN_OP, "*");
    push(calc->stack, elem);

    if (0 != (ret = get_token(calc))) { return ret; }

    switch (calc->cur_t)
    {
        case VAR:
        case INT:
        case FLOAT:
            if (0 != (ret = mul(calc))) { return ret; }
            elem = elem_str(BIN_OP, "*");
            push(calc->stack, elem);
            return 0;
        default:
            return 0;
    }
}


int term (Calc *calc)
{
    int ret;

    if (0 != (ret = get_token(calc))) { return ret; }

    if (calc->cur_t == OPEN_BRACKET)
    {
        if (0 != (ret = mul(calc))) { return ret; }

        if (0 != (ret = get_token(calc))) { return ret; }

        switch (calc->cur_t)
        {
            case VAR:
            case INT:
            case FLOAT:
                if (0 != (ret = mul(calc))) { return ret; }
                Elem *elem = elem_str(BIN_OP, "*");
                push(calc->stack, elem);
                break;
            default:
                break;
        }
    }
    else
    {
        if (0 != (ret = mul(calc))) { return ret; }
    }

    while (1)
    {
        if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

        switch (calc->cur_t)
        {
            case MUL:
            case DIV:
            {
                Elem *elem = elem_str(BIN_OP, calc->cur_str);

                up_used(calc);

                if (0 != (ret = mul(calc))) { free(elem); return ret; }

                push(calc->stack, elem);

                continue;
            }
            case OPEN_BRACKET:
            {
                if (0 != (ret = term_spec(calc))) { return ret; }

                if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

                switch (calc->cur_t)
                {
                    case OPEN_BRACKET:
                        continue;
                    case INT:
                    case FLOAT:
                    case VAR:
                    {
                        if (0 != (ret = mul(calc))) { return ret; }

                        Elem *elem = elem_str(BIN_OP, calc->cur_str);

                        push(calc->stack, elem);

                        continue;
                    }
                    default:
                        return 0;
                }
            }
            default:
                return 0;
        }
    }
}


int mul (Calc *calc)
{
    int ret;

    if (0 != (ret = get_token(calc))) { return ret; }

    if (calc->cur_t == END) { return EMPTY_MUL; }

    up_used(calc);

    switch (calc->cur_t)
    {
        case PLUS:
        case MINUS:
        {
            Elem *elem = elem_str(UN_OP, calc->cur_str);

            if (0 != (ret = mul(calc))) { free(elem); return ret; }

            push(calc->stack, elem);

            return 0;
        }
        case OPEN_BRACKET:
        {
            if (0 != (ret = expression(calc))) { return ret; }

            if (0 != (ret = get_token(calc))) { return ret; }

            if (calc->cur_t != CLOSE_BRACKET) { return MISS_CLOSE_PARENT; }

            up_used(calc);

            return 0;
        }
        case INT:
        {
            int_type val = strtoll(calc->cur_str, NULL, 10);
            Elem *elem = elem_int(val);

            push(calc->stack, elem);

            return 0;
        }
        case FLOAT:
        {
            float_type val = strtod(calc->cur_str, NULL);
            Elem *elem = elem_float(val);

            push(calc->stack, elem);

            return 0;
        }
        case VAR:
        {
            Elem *elem = elem_str(VAR_NAME, calc->cur_str);
            push(calc->stack, elem);
            return 0;
        }
        case CLOSE_BRACKET:
        {
            return EMPTY_MUL;
        }
        default:
        {
            return MUL_UNEXPECT;
        }
    }
}


int get_token(Calc *calc)
{
    if (!calc->was_used)
    {
        return 0;
    }

    down_used(calc);

    int cur;

    while (cur = fgetc(calc->in_stream), isspace(cur) && cur != '\n' && cur != EOF);

    if (cur == EOF || cur == '\n')
    {
        strcpy(calc->cur_str, "\n");
        calc->cur_t = END;
        return 0;
    }
    else if (isalpha(cur) || cur == '_')
    {
        // read variable name
        ungetc(cur, calc->in_stream);

        get_var(calc->in_stream, calc->cur_str, MAX_NAME_LEN, (int*)&(calc->cur_t));

        if (calc->cur_t == -1)
        {
            calc->cur_t = WRONG;
            return VAR_NAME_ERROR;
        }

        return 0;
    }
    else if (isdigit(cur) || cur == '.')
    {
        // read number
        ungetc(cur, calc->in_stream);

        get_num(calc->in_stream, calc->cur_str, MAX_TOKEN_LEN, (int*)&(calc->cur_t));

        if (calc->cur_t == -1 || (calc->cur_str[0] == '.' && strlen(calc->cur_str) == 1))
        {
            calc->cur_t = WRONG;
            return NUMBER_TOKEN_ERROR;
        }

        return 0;
    }
    else
    {
        calc->cur_str[0] = (char) cur;
        calc->cur_str[1] = '\0';

        switch (cur)
        {
            case '(':
                calc->cur_t = OPEN_BRACKET;
                break;
            case ')':
                calc->cur_t = CLOSE_BRACKET;
                break;
            case '*':
                calc->cur_t = MUL;
                break;
            case '/':
                calc->cur_t = DIV;
                break;
            case '+':
                calc->cur_t = PLUS;
                break;
            case '-':
                calc->cur_t = MINUS;
                break;
            default:
                // unexpected
                calc->cur_t = WRONG;
                return INVALID_TOKEN;
        }

        return 0;
    }
}


void set_in_stream(Calc *calc, FILE *new_stream)
{
    calc->in_stream = new_stream;
}


void set_out_stream(Calc *calc, FILE *new_stream)
{
    calc->out_stream = new_stream;
}


void set_err_stream(Calc *calc, FILE *new_stream)
{
    calc->err_stream = new_stream;
}


int up_used(Calc *calc)
{
    if (!calc->was_used)
    {
        calc->was_used = 1;
        return 0;
    } else
    {
        // already up
        return 1;
    }
}


int down_used(Calc *calc)
{
    if (calc->was_used)
    {
        calc->was_used = 0;
        return 0;
    } else
    {
        // already down
        return 1;
    }
}


int process_new_vars(Calc *calc, InputVariable type)
{
    Stack *buf = init_stack();

    long rem = calc->vars->names->size;

    for (long i = 0; i < calc->stack->size; i++)
    {
        if (calc->stack->stack[i]->type == VAR_NAME &&
                -1 == find_var(calc->vars, calc->stack->stack[i]->data))
        {
            add_var(calc->vars, calc->stack->stack[i], NULL);
            push(buf, calc->stack->stack[i]);
        }
    }

    calc->vars->names->size = rem;
    calc->vars->values->size = rem;

    qsort(buf->stack, buf->size, sizeof(Elem*), (comp_t) comp_elem_str);

    for (long i = 0; i < buf->size; i++)
    {
        Elem *elem = buf->stack[i];
        Elem *value;

        Calc *sub_calc = init_calc();
        set_in_stream(sub_calc, calc->in_stream);
        set_out_stream(sub_calc, calc->out_stream);
        set_err_stream(sub_calc, calc->err_stream);

        if (type == ALLOW_EXPR)
        {
            delete_var_set(sub_calc->vars);
            sub_calc->vars = calc->vars;

            do {
                fprintf(sub_calc->out_stream, "%s=", elem->data);

                clear(sub_calc->stack);

                int ret;

                if (0 != (ret = build_postfix(sub_calc)))
                {
                    error(calc, ret);
                }

                if (ret == 0 && 0 != (ret = process_new_vars(sub_calc, ALLOW_EXPR)))
                {
                    error(calc, ret);
                }

                if (ret == 0 && 0 != (ret = exec_stack(sub_calc)))
                {
                    error(calc, ret);
                }

                if (sub_calc->result->type == INT_NUM ||
                    sub_calc->result->type == FLOAT_NUM)
                {
                    value = sub_calc->result;
                    sub_calc->result = NULL;
                    break;
                }

                fprintf(calc->err_stream, "It isn't INT or FLOAT value\n");
                print_elem(sub_calc->result, sub_calc->err_stream);
            } while(1);

            sub_calc->vars = NULL;
        }
        else
        {
            int ret = get_token(sub_calc);
            if (sub_calc->cur_t == MINUS)
            {
                up_used(sub_calc);
                ret = get_token(sub_calc);
                memmove(sub_calc->cur_str + 1, sub_calc->cur_str, strlen(sub_calc->cur_str) + 1);
                sub_calc->cur_str[0] = '-';
            }
            up_used(calc);
            if (sub_calc->cur_t != END) get_token(calc);

            if (calc->cur_t != END || ret != 0 || (sub_calc->cur_t != INT && sub_calc->cur_t != FLOAT))
            {
                delete_calc(sub_calc);
                fill(buf, NULL);
                delete_stack(buf);
                return NUMBER_TOKEN_ERROR;
            }

            if (sub_calc->cur_t == INT)
                value = elem_int(strtoll(sub_calc->cur_str, NULL, 10));
            else
                value = elem_float(strtod(sub_calc->cur_str, NULL));
        }

        add_var(calc->vars, copy_elem(buf->stack[i]), value);
        delete_calc(sub_calc);
    }

    fill(buf, NULL);
    delete_stack(buf);
    return 0;
}


Elem *replace_var(Calc *calc, Elem *elem)
{
    switch (elem->type)
    {
        case VAR_NAME:
        {
            long ind = find_var(calc->vars, elem->data);

            if (ind != -1)
                return calc->vars->values->stack[ind];
            else
                return NULL;
        }
        default:
            return elem;
    }
}


int exec_stack(Calc *calc)
{
    Stack *buf = init_stack();

    for (long i = 0; i < calc->stack->size; i++)
    {
        Elem *cur = calc->stack->stack[i];

        cur = replace_var(calc, cur);

        if (cur == NULL)
        {
            return EXEC_STACK_ERR;
        }

        switch (cur->type)
        {
            case BIN_OP:
            {
                Elem *b = copy_elem(top(buf));
                pop(buf);
                Elem *a = copy_elem(top(buf));
                pop(buf);

                if (a->type == FLOAT_NUM || b->type == FLOAT_NUM)
                {
                    float_type fa, fb;

                    if (a->type == INT_NUM)
                    {
                        fa = (float_type)*(int_type*)a->data;
                    }
                    else
                    {
                        fa = *(float_type*)a->data;
                    }

                    if (b->type == INT_NUM)
                    {
                        fb = (float_type)*(int_type*)b->data;
                    }
                    else
                    {
                        fb = *(float_type*)b->data;
                    }

                    if (0 == strcmp((char*)cur->data, "+"))
                    {
                        fa += fb;
                    } else if (0 == strcmp((char*)cur->data, "-"))
                    {
                        fa -= fb;
                    } else if (0 == strcmp((char*)cur->data, "*"))
                    {
                        fa *= fb;
                    } else if (0 == strcmp((char*)cur->data, "/"))
                    {
                        if ((fb < 0 && -1 * fb < comp) || (fb >= 0 && fb < comp))
                        {
                            free(a);
                            free(b);
                            delete_stack(buf);
                            return ZERO_DIVIDE;
                        }

                        fa /= fb;
                    } else
                    {
                        // unexpect
                    }

                    if (a->type == FLOAT_NUM)
                    {
                        *(float_type*)(a->data) = fa;
                        push(buf, a);
                        free(b);
                    }
                    else
                    {
                        *(float_type*)(b->data) = fa;
                        push(buf, b);
                        free(a);
                    }
                }
                else
                {
                    int_type fa = *(int_type*)a->data;
                    int_type fb = *(int_type*)b->data;

                    if (0 == strcmp((char*)cur->data, "+"))
                    {
                        fa += fb;
                    } else if (0 == strcmp((char*)cur->data, "-"))
                    {
                        fa -= fb;
                    } else if (0 == strcmp((char*)cur->data, "*"))
                    {
                        fa *= fb;
                    } else if (0 == strcmp((char*)cur->data, "/"))
                    {
                        if (fb == 0)
                        {
                            free(a);
                            free(b);
                            delete_stack(buf);
                            return ZERO_DIVIDE;
                        }

                        fa /= fb;
                    } else
                    {
                        // unexpect
                    }

                    if (fa > 2147483647 || fa < -2147483648)
                    {
                        free(a);
                        free(b);
                        delete_stack(buf);
                        return ERR_ERANGE;
                    }

                    *(int_type*)(a->data) = fa;
                    a->type = INT_NUM;

                    push(buf, a);
                    free(b);
                }

                break;
            }
            case UN_OP:
            {
                if (0 == strcmp((char*)cur->data, "-"))
                {
                    Elem *elem = top(buf);

                    switch (elem->type)
                    {
                        case INT_NUM:
                            *(int_type*)elem->data *= -1;
                            break;
                        case FLOAT_NUM:
                            *(float_type*)elem->data *= -1;
                            break;
                        default:
                            //unexpect
                            break;
                    }
                }
                else { /* unexpect */ }
                break;
            }
            case INT_NUM:
            case FLOAT_NUM:
            {
                Elem *elem = copy_elem(cur);
                push(buf, elem);
                break;
            }
            default:
            {
                // unexpect
                break;
            }
        }
    }

    if (calc->result != NULL)
    {
        free(calc->result);
    }

    calc->result = copy_elem(buf->stack[0]);

    delete_stack(buf);

    return 0;
}


void print_stack(Stack *stack, FILE *stream)
{
    for (long i = stack->size - 1; i >= 0; i--)
    {
        print_elem(stack->stack[i], stream);
    }
}


void print_elem(Elem *elem, FILE *stream)
{
    switch (elem->type)
    {
        case INT_NUM:
            fprintf(stream, "INT: %lld\n", *(int_type*)(elem->data));
            break;
        case FLOAT_NUM:
            fprintf(stream, "FLOAT: %f\n", *(float_type*)(elem->data));
            break;
        case BIN_OP:
            fprintf(stream, "BINARY operand: %s\n", elem->data);
            break;
        case UN_OP:
            fprintf(stream, "UNARY operand: %s\n", elem->data);
            break;
        case VAR_NAME:
            fprintf(stream, "VAR: %s\n", elem->data);
            break;
        case ERROR:
            fprintf(stream, "Type: ERROR\nMSG: %s\n", elem->data);
            break;
        default:
            fprintf(stream, "Trouble\n");
    }
}


Elem* copy_elem(Elem *elem)
{
    if (elem == NULL)
    {
        return NULL;
    }
    else
    {
        size_t size = sizeof (Elem);

        switch (elem->type)
        {
            case INT_NUM:
            {
                size += sizeof (int_type);
                break;
            }
            case FLOAT_NUM:
            {
                size += sizeof (float_type);
                break;
            }
            case BIN_OP:
            case UN_OP:
            case VAR_NAME:
            {
                size += strlen((char*)elem->data) + 1;
                break;
            }
            default:
            {
                // unexpect
                return NULL;
            }
        }

        Elem *new_elem = malloc(size);
        memcpy(new_elem, elem, size);

        return new_elem;
    }
}


Elem* elem_str(StackType type, char *str)
{
    ulong len = strlen(str);
    Elem *elem = malloc(sizeof(Elem) + sizeof(char) * (len + 1));
    memcpy(elem->data, str, len + 1);
    elem->type = type;

    return elem;
}


Elem* elem_int(int_type val)
{
    Elem *elem = malloc(sizeof (Elem) + sizeof (int_type));
    elem->type = INT_NUM;
    *(int_type*)(elem->data) = val;

    return elem;
}

Elem* elem_float(float_type val)
{
    Elem *elem = malloc(sizeof (Elem) + sizeof (float_type));
    elem->type = FLOAT_NUM;
    *(float_type*)(elem->data) = val;

    return elem;
}


void get_num(FILE *stream, char* dest, int max_len, int *what)
{
    int cur;
    int len = 0;
    int is_end = 0;
    int before_dot = 1;

    while (cur = fgetc(stream), isspace(cur) && cur != '\n' && cur != EOF);
    if (cur != EOF) ungetc(cur, stream);

    while (!is_end)
    {
        if (len >= max_len)
        {
            dest[len] = '\0';
            *what = -1;
            return;
        }

        cur = fgetc(stream);

        if (isdigit(cur))
        {
            dest[len++] = (char) cur;
        }
        else if (cur == '.' && before_dot)
        {
            before_dot = 0;
            dest[len++] = (char) cur;
        }
        else
        {
            is_end = 1;
        }
    }

    if (cur != EOF) ungetc(cur, stream);

    dest[len] = '\0';

    if (len == 0)
    {
        *what = -1;
        return;
    }

    char *end;
    *what = before_dot ? INT : FLOAT;
    if (*what == INT)
    {
        int_type val = strtoll(dest, &end, 10);
        if (val > 2147483647ll || val < -2147483648ll)
        {
            *what = -1;
            return;
        }
    }
    else
        strtod(dest, &end);

    if (errno == ERANGE)
    {
        *what = -1;
        return;
    }
}


void get_var (FILE *stream, char* dest, int max_len, int *what)
{
    int cur;
    int len = 0;

    while (1)
    {
        cur = fgetc(stream);

        if (len > max_len || (len == 0 && isdigit(cur)))
        {
            if (cur != EOF) ungetc(cur, stream);
            *what = -1;
            dest[len] = '\0';
            return;
        }

        if (!(isdigit(cur) || isalpha(cur) || cur == '_'))
        {
            if (cur != EOF) ungetc(cur, stream);
            dest[len] = '\0';
            break;
        }

        dest[len++] = (char) cur;
    }

    *what = VAR;
}


long miss_line(Calc *calc)
{
    long count = 0;
    int cur;

    while (cur = getc(calc->in_stream), cur != '\n' && cur != EOF)
    {
        count++;
    }

    return count;
}


void put_str_back(Calc *calc, char *str)
{
    long len = (long) strlen(str);

    for (long i = len - 1; i >= 0; i--)
    {
        ungetc(str[i], calc->in_stream);
    }
}


Elem* check_var (Calc *calc)
{
    up_used(calc);
    get_token(calc);
    up_used(calc);

    if (calc->cur_t != VAR)
    {
        put_str_back(calc, calc->cur_str);
        return NULL;
    }

    Elem *name = elem_str(VAR_NAME, calc->cur_str);

    get_token(calc);
    up_used(calc);

    if (0 != strcmp(calc->cur_str, "="))
    {
        put_str_back(calc, calc->cur_str);
        put_str_back(calc, name->data);
        free(name);
        return NULL;
    }

    return name;
}


void print_var_set(VarSet *set, FILE *stream)
{
    long size = set->names->size;

    for (int i = 0; i < size; i++)
    {
        fprintf(stream, "%*d. NAME: %*s, ", 3, i + 1, MAX_NAME_LEN, set->names->stack[i]->data);
        fprintf(stream, "VALUE: ");
        print_elem(set->values->stack[i], stream);
    }
}


void print_dict(Calc *calc, FILE *stream)
{
    if (calc->vars->names->size <= 0)
    {
        fprintf(stream, "Variable dictionary is empty\n");
    }
    else
    {
        fprintf(stream, "\nDictionary:\n");
        print_var_set(calc->vars, stream);
        fprintf(stream, "\n");
    }
}


int check_str_input(Calc *calc, char *src)
{
    char buf[100];

    int cur;

    while (cur = fgetc(calc->in_stream), isspace(cur) && cur != '\n' && cur != EOF);

    ulong req_len = strlen(src);
    int len;

    if (cur == '\n')
    {
        ungetc('\n', calc->in_stream);
        return -1;
    } else if (cur == EOF)
    {
        return -1;
    } else
    {
        buf[0] = (char) cur;
        len = 1;
    }

    while (cur = fgetc(calc->in_stream), len < req_len && cur != '\n' && cur != EOF)
    {
        buf[len++] = (char) cur;
    }

    buf[len] = '\0';

    if (cur != EOF) ungetc(cur, calc->in_stream);

    if (len < req_len || 0 != strcmp(src, buf))
    {
        put_str_back(calc, buf);
        return -1;
    }
    else
    {
        return 0;
    }
}


Stack* copy_stack(Stack *stack)
{
    if (stack == NULL)
    {
        return NULL;
    }

    Stack *new_stack = init_stack();

    for (long i = 0; i < stack->size; i++)
    {
        push(new_stack, copy_elem(stack->stack[i]));
    }

    return new_stack;
}


int comp_elem_str(Elem **a, Elem **b)
{
    return strcmp((*a)->data, (*b)->data);
}
