#include "calc_stack.h"

typedef struct {
    Stack *names;
    Stack *values;
} VarSet;


VarSet* init_var_set(void);
void delete_var_set(VarSet *set);

void add_var(VarSet *set, Elem *name, Elem *value);

void delete_var(VarSet *set, long i);

long find_var(VarSet *set, char *str);
