heap_test: tests/heap.test.c src/heap.c src/file.c include/heap.h include/file.h
	gcc -o heap_test tests/heap.test.c src/heap.c src/file.c -lcheck -lm -lsubunit

file_test: tests/file.test.c src/heap.c src/file.c include/heap.h include/file.h
	gcc -o file_test tests/file.test.c src/heap.c src/file.c -lcheck -lm -lsubunit

main: main.c src/heap.c src/file.c include/heap.h include/file.h
	gcc -o main main.c src/heap.c src/file.c

clean:
	rm -f heap_test file_test main

run_heap_test: heap_test
	./heap_test

run_file_test: file_test
	./file_test

run_main: main
	./main
