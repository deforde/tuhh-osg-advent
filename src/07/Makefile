a.out: main.o
	gcc main.o -o $@ -fsanitize=address,undefined -lpthread

main.o:
	gcc -Wall -Wextra -Wpedantic -Werror -MMD -MP -g3 -D_FORTIFY_SOURCE=2 -fsanitize=address,undefined -c main.c -o $@

.PHONY: clean run

clean:
	rm a.out main.o main.d

run: a.out
	seq 1 100 | ./a.out "grep 1[1-3]" "grep [1-3]2"

-include main.d
