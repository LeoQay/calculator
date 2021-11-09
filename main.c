#include <stdio.h>

#include "calc.h"


int main(int argc, char *argv[])
{
    FILE *input = stdin;

    if (argc > 1)
    {
        input = fopen(argv[1], "r+");
    }

    /////////////////////////////
    /// calculating

    Calc *calc = init_calc();

    set_stream(calc, input);

    input_mgr(calc);

    delete_calc(calc);

    /////////////////////////////

    if (argc > 1)
    {
        fclose(input);
    }

    return 0;
}



