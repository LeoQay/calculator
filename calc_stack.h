typedef enum {
    BIN_OP,    // binary operator
    UN_OP,     // unary operator
    INT_NUM,   // integer value
    FLOAT_NUM, // float value
    VAR_NAME,  // variable name
    ERROR      // undefined
} StackType;


typedef struct {
    StackType type;
    char data[];
} Elem;


typedef struct {
    Elem **stack;

    long size;
    long r_size;
} Stack;


Stack* init_stack(void);
void delete_stack(Stack *stack);
void push(Stack *stack, Elem *elem);
Elem* top(Stack *stack);
void pop(Stack *stack);
void clear(Stack *stack);

