#include <stdlib.h>

#include "calc_stack.h"


Stack* init_stack(void)
{
    Stack *new_stack = malloc(sizeof(Stack));

    new_stack->size = 0;
    new_stack->r_size = 10;

    new_stack->stack = malloc(sizeof(Elem*) * new_stack->r_size);

    return new_stack;
}


void delete_stack(Stack *stack)
{
    if (stack == NULL)
    {
        return;
    }

    for (long i = 0; i < stack->size; i++)
    {
        free(stack->stack[i]);
    }

    free(stack->stack);
    free(stack);
}


void push(Stack *stack, Elem *elem)
{
    if (stack->size >= stack->r_size)
    {
        stack->r_size <<= 1;
        stack->stack = realloc(stack->stack,sizeof(Elem*) * stack->r_size);
    }

    stack->stack[stack->size++] = elem;
}


Elem* top(Stack *stack)
{
    if (stack->size >= 1)
    {
        return stack->stack[stack->size - 1];
    } else
    {
        return NULL;
    }
}


void pop(Stack *stack)
{
    if (stack->size >= 1)
    {
        stack->size--;

        free(stack->stack[stack->size]);

        if ((stack->r_size > 20) && (stack->size < (stack->r_size >> 1)))
        {
            stack->r_size >>= 1;
            stack->stack = realloc(stack->stack,sizeof(Elem*) * stack->r_size);
        }
    } else
    {
        return;
    }
}


void clear(Stack *stack)
{
    for (long i = 0; i < stack->size; i++)
    {
        free(stack->stack[i]);
    }

    stack->r_size = 10;
    stack->size = 0;

    stack->stack = realloc(stack->stack, sizeof(Elem*) * stack->r_size);
}


void fill(Stack *stack, Elem *elem)
{
    for (long i = 0; i < stack->size; i++)
    {
        stack->stack[i] = elem;
    }
}