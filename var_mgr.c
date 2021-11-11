#include <stdlib.h>
#include <string.h>

#include "var_mgr.h"

VarSet* init_var_set(void)
{
    VarSet *set = malloc(sizeof (VarSet));

    set->values = init_stack();
    set->names  = init_stack();

    return set;
}


void delete_var_set(VarSet *set)
{
    if (set == NULL) return;

    delete_stack(set->names);
    delete_stack(set->values);

    free(set);
}


void add_var(VarSet *set, Elem *name, Elem *value)
{
    long check = find_var(set, name->data);

    if (check != -1)
    {
        delete_var(set, check);
    }

    push(set->names, name);
    push(set->values, value);
}


void delete_var(VarSet *set, long i)
{
    long size = set->names->size;

    if (i < 0 || i >= size)
    {
        return;
    }

    if (set->names->size == 1)
    {
        pop(set->names);
        pop(set->values);
    }
    else
    {
        free(set->names->stack[i]);
        free(set->values->stack[i]);
        set->names->stack[i] = set->names->stack[size - 1];
        set->values->stack[i] = set->values->stack[size - 1];
        set->names->stack[size - 1] = NULL;
        set->values->stack[size - 1] = NULL;
        pop(set->names);
        pop(set->values);
    }
}


long find_var(VarSet *set, char *str)
{
    Elem **target = set->names->stack;

    for (int i = 0; i < set->names->size; i++)
    {
        if (strcmp(target[i]->data, str) == 0)
        {
            return i;
        }
    }

    return -1;
}
