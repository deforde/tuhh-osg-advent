a.out: main.o
	gcc main.o -o $@ -fsanitize=address,undefined

main.o:
	gcc -Wall -Wextra -Wpedantic -Werror -MMD -MP -g3 -D_FORTIFY_SOURCE=2 -fsanitize=address,undefined -c main.c -o $@

.PHONY: clean run

clean:
	rm a.out main.o main.d

run: a.out
	echo "test" > test.txt
	./a.out test.txt
	./a.out test.txt
	echo "t" >> test.txt
	./a.out test.txt | exit 0
	./a.out -r test.txt
	./a.out test.txt
	./a.out test.txt
	rm test.txt

-include main.d
