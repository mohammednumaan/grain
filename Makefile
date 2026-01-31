heap_test: tests/heap.test.c src/heap.c src/file.c include/heap.h include/file.h
	gcc -o heap_test tests/heap.test.c src/heap.c src/file.c -lcheck -lm -lsubunit

clean:
	rm -f heap_test

run: heap_test
	./heap_test
