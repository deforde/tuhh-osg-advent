cat: main.o
	gcc main.o -o $@ -fsanitize=address,undefined

main.o:
	gcc -Wall -Wextra -Wpedantic -Werror -MMD -MP -g3 -D_FORTIFY_SOURCE=2 -fsanitize=address,undefined -c main.c -o $@

.PHONY: clean

clean:
	rm cat main.o main.d

-include main.d
