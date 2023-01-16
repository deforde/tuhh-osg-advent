a.out: main.o
	gcc main.o -o $@

main.o:
	gcc -Wall -Wextra -Wpedantic -Werror -MMD -MP -g3 -c main.c -o $@

.PHONY: clean run

clean:
	rm a.out main.o main.d

run: a.out
	./a.out

-include main.d
