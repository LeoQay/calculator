Dmitriev Leonid 207

Originally, task was decided with functions
that not need in tests now.

For the tests, a simplified function was created that calls
the necessary functions that have already been written.

Such as:

(@end)
To stop endless cycle
    Examples:
    >>> @end
    Finish state:
    Variable dictionary is empty

In the expression, it is allowed to skip the multiplication
sign before and after the brackets;
    Examples:
        >>> 2 (1 + 2)
        INT: 6
        >>> (2. * 1) 3
        FLOAT: 6.000000
        >>> (1 + 2) (2 + 3)
        INT: 15

You can add and change variables by this way
    Rule:
        >>> <variable name> = <expression>
    Examples:
        >>> a = 1
        >>> b = 1
        >>> a + b
        INT: 2
        >>> a = (1.0 + 1) * 1
        a + b
        FLOAT: 3.000000

An unlimited number of unary plus and minus signs are allowed
    Examples:
        >>> +++1
        INT: 1
        >>> ---1
        INT: -1
        >>> -+-1
        INT: 1

(@save, @use)
Added the ability to recalculate expressions with new variable values
(or with old ones) (Only one expression can be stored at a time)
    Examples:
        >>> a = 1
        >>> a = 2
        >>> a + b
        INT: 3
        >>> @save
        >>> a = 125.0
        >>> @use
        FLOAT: 127.000000

(@dict)
You can also print the current dictionary of variables
    Examples:
        >>> @dict
        Variable dictionary is empty
        >>> a = 1
        >>> b = 12.2
        >>> @dict
        Dictionary:
          1. NAME:      a, VALUE: INT: 1
          2. NAME:      b, VALUE: FLOAT: 12.200000



SIMPLE CALC LANGUAGE

<expression> = <term> [<3L sign> <term>]

<1L sign> = <+>, <-> // unary
<2L sign> = <*>, </> // binary
<3L sign> = <+>, <-> // binary

<term> = <term spec> [<term cont>]
<term> = <mul> [<term cont>]

<term cont> = <2L sign> <mul>
<term cont> = <term spec>

<term spec> = (<expression>)<val>
<term spec> = (<expression>)<variable>
<term spec> = (<expression>)

<mul> = <1L sign> <mul>
<mul> = (<expression>)
<mul> = <val>
<mul> = <variable>

<val> = <int>
<val> = <float>

<float> = <int>.[<int>]

<int> = <num>[<num>]

<num> = {0,1,2,3,4,5,6,7,8,9}

<variable> = <start correct sym> [<correct sym>]

<start correct sym> = <word> or '_'
<correct sym> = <word> or <num> or '_'
<word> = {a .. z, A .. Z}
