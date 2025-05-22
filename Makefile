all:
	gcc -Wall -O0 user_alloc.c -o user_alloc
	gcc -Wall -O0 test_pte_meta.c -o test_pte_meta
clean:
	rm -f user_alloc test_pte_meta