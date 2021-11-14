TARGET=main
NAME=solution

all: $(TARGET).o calc.o calc_stack.o var_mgr.o
	gcc $(TARGET).o calc.o calc_stack.o var_mgr.o -o $(NAME)

$(TARGET).o: $(TARGET).c calc.h
	gcc $(TARGET).c -c

calc.o: calc.c calc.h var_mgr.h
	gcc calc.c -c

var_mgr.o: var_mgr.c var_mgr.h calc_stack.h
	gcc var_mgr.c -c


calc_stack.o: calc_stack.c calc_stack.h
	gcc calc_stack.c -c

clean:
	rm -f *.o $(NAME)

run:
	./$(NAME)

run_vg:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)

