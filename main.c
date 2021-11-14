#include <stdio.h>

#include "calc.h"


int main(void)
{
    Calc *calc = init_calc();

    set_in_stream(calc, stdin);
    set_out_stream(calc, stdout);
    set_err_stream(calc, stderr);

    
    int ret = ejudge_process_str(calc);

    //input_mgr(calc);

    delete_calc(calc);

    return ret;
}



