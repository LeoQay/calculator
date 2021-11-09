#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "calc.h"


Calc* init_calc ()
{
    Calc *calc = malloc(sizeof (Calc));
    calc->cur_t = END;
    calc->was_used = 1;
    calc->stream = stdin;
    calc->stack = init_stack();
    calc->result = NULL;


    calc->vars = init_var_set();

    Elem *name = elem_str(VAR_NAME, "E");
    Elem *value = elem_float(2.7182818284590452354);
    add_var(calc->vars, name, value);

    name = elem_str(VAR_NAME, "PI");
    value = elem_float(3.14159265358979323846);
    add_var(calc->vars, name, value);

    return calc;
}


void delete_calc(Calc *calc)
{
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
            printf("Finish state:\n");
            is_end = 1;
        }

        printf("\nDictionary:\n");
        print_var_set(calc->vars);
        printf("\n");
    } while (!is_end);
}


int process_str (Calc *calc)
{
    Elem *var_name;

    if (0 == check_end(calc))
    {
        return -1;
    }

    if (NULL != (var_name = check_var(calc)))
    {
        do_calc(calc);

        if (calc->result->type != INT_NUM && calc->result->type != FLOAT_NUM)
        {
            printf("I get not value\n");
            print_elem(calc->result);
        }
        else
        {
            add_var(calc->vars, var_name, calc->result);
            calc->result = NULL;
        }
    }
    else
    {
        do_calc(calc);

        print_elem(calc->result);
    }

    return 0;
}


void do_calc(Calc *calc)
{
    clear(calc->stack);

    int ret = expression(calc);

    if (ret != 0 || calc->cur_t != END)
    {
        if (calc->cur_t != END) miss_line(calc);

        if (calc->result != NULL) free(calc->result);

        switch (ret)
        {
            case EXPR_UNEXPECT:
            case INVALID_OP:
                calc->result = elem_str(ERROR, "Invalid operation");
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
            case MISS_PARENT:
                calc->result =
                        elem_str(ERROR,
                                 "The balance of brackets is broken");
                break;
            case MUL_UNEXPECT:
            case TERM_SPEC_ERR:
                calc->result = elem_str(ERROR, "Impossible to reach it");
                break;
            default:
                printf("ret = %d\n", ret);
                calc->result = elem_str(ERROR, "Error!");
                break;
        }

        clear(calc->stack);
        return;
    }

    exec_stack(calc);

    clear(calc->stack);
}


int expression (Calc *calc)
{
    int ret;

    if (0 != (ret = term(calc))) { return ret; }

    while (1)
    {
        if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

        switch (calc->cur_t)
        {
            case CLOSE_BRACKET:
                return 0;
            case END:
                up_used(calc);
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

    if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

    if (calc->cur_t != OPEN_BRACKET) { up_used(calc); return TERM_SPEC_ERR; }

    if (0 != (ret = mul(calc))) { return ret; }

    Elem *elem = elem_str(BIN_OP, "*");
    push(calc->stack, elem);

    if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

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

    if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

    if (calc->cur_t == OPEN_BRACKET)
    {
        if (0 != (ret = mul(calc))) { return ret; }

        if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

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

    if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

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

            if (0 != (ret = get_token(calc))) { up_used(calc); return ret; }

            if (calc->cur_t != CLOSE_BRACKET) { up_used(calc); return MISS_PARENT; }

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
            float_type val = strtold(calc->cur_str, NULL);
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
            up_used(calc);
            return EMPTY_MUL;
        }
        default:
        {
            up_used(calc);
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

    while (cur = fgetc(calc->stream), isspace(cur) && cur != '\n' && cur != EOF);

    if (cur == EOF || cur == '\n')
    {
        strcpy(calc->cur_str, "\n");
        calc->cur_t = END;
        return 0;
    }
    else if (isalpha(cur) || cur == '_')
    {
        // read variable name
        ungetc(cur, calc->stream);

        get_var(calc->stream, calc->cur_str, MAX_NAME_LEN, (int*)&(calc->cur_t));

        if (calc->cur_t == -1)
        {
            return VAR_NAME_ERROR;
        }

        return 0;
    }
    else if (isdigit(cur) || cur == '.')
    {
        // read number
        ungetc(cur, calc->stream);

        get_num(calc->stream, calc->cur_str, MAX_TOKEN_LEN, (int*)&(calc->cur_t));

        if (calc->cur_t == -1)
        {
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
                return INVALID_OP;
        }

        return 0;
    }
}


void set_stream(Calc *calc, FILE *new_stream)
{
    calc->stream = new_stream;
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


Elem *replace_var(Calc *calc, Elem *elem)
{
    switch (elem->type)
    {
        case VAR_NAME:
        {
            long ind = find_var(calc->vars, elem->data);

            if (ind != -1)
            {
                return calc->vars->values->stack[ind];
            }

            Elem *return_elem;

            Calc *sub_calc = init_calc();
            delete_var_set(sub_calc->vars);
            sub_calc->vars = calc->vars;

            set_stream(sub_calc, stdin);

            do {
                printf("%s=", elem->data);

                clear(sub_calc->stack);

                do_calc(sub_calc);

                if (sub_calc->result->type == INT_NUM ||
                sub_calc->result->type == FLOAT_NUM)
                {
                    return_elem = sub_calc->result;
                    sub_calc->result = NULL;
                    break;
                }

                printf("It isn't INT or FLOAT value\n");
                print_elem(sub_calc->result);
            } while(1);

            add_var(sub_calc->vars, elem_str(VAR_NAME, elem->data), return_elem);

            sub_calc->vars = NULL;
            delete_calc(sub_calc);

            return return_elem;
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
                        fa /= fb;
                    } else
                    {
                        // unexpect
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


void print_stack(Stack *stack)
{
    for (long i = stack->size - 1; i >= 0; i--)
    {
        print_elem(stack->stack[i]);
    }
}


void print_elem(Elem *elem)
{
    switch (elem->type)
    {
        case INT_NUM:
            printf("INT: %lld\n", *(int_type*)(elem->data));
            break;
        case FLOAT_NUM:
            printf("FLOAT: %Lg\n", *(float_type*)(elem->data));
            break;
        case BIN_OP:
            printf("BINARY operand: %s\n", elem->data);
            break;
        case UN_OP:
            printf("UNARY operand: %s\n", elem->data);
            break;
        case VAR_NAME:
            printf("VAR: %s\n", elem->data);
            break;
        case ERROR:
            printf("Type: ERROR\nMSG: %s\n", elem->data);
            break;
        default:
            printf("Trouble\n");
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

    if (len == 0)
    {
        dest[0] = '\0';
        *what = -1;
        return;
    }

    *what = before_dot ? INT : FLOAT;

    dest[len] = '\0';
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


void miss_line(Calc *calc)
{
    int cur;

    while (cur = getc(calc->stream), cur != '\n' && cur != EOF);
}


void put_str_back(Calc *calc, char *str)
{
    long len = (long) strlen(str);

    for (long i = len - 1; i >= 0; i--)
    {
        ungetc(str[i], calc->stream);
    }
}


Elem* check_var (Calc *calc)
{
    up_used(calc);
    get_token(calc);
    up_used(calc);

    if (calc->cur_t != VAR)
    {
        if (calc->cur_t != END) put_str_back(calc, calc->cur_str);
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


void print_var_set(VarSet *set)
{
    long size = set->names->size;

    for (int i = 0; i < size; i++)
    {
        printf("%d. NAME: %*s, ", i + 1, MAX_NAME_LEN, set->names->stack[i]->data);
        printf("VALUE: ");
        print_elem(set->values->stack[i]);
    }
}


int check_end(Calc *calc)
{
    char end[] = "@end";
    char buf[5];

    int cur;

    while (cur = fgetc(calc->stream), isspace(cur) && cur != '\n' && cur != EOF);

    int req_len = 4;
    int len = 0;

    if (cur == '\n')
    {
        ungetc('\n', calc->stream);
        return -1;
    } else if (cur == EOF)
    {
        return -1;
    } else
    {
        buf[0] = (char) cur;
        len = 1;
    }

    while (cur = fgetc(calc->stream), len < req_len && cur != '\n' && cur != EOF)
    {
        buf[len++] = (char) cur;
    }

    buf[len] = '\0';

    if (cur != EOF) ungetc(cur, calc->stream);

    if (len < req_len || 0 != strcmp(end, buf))
    {
        put_str_back(calc, buf);
        return -1;
    }
    else
    {
        return 0;
    }
}