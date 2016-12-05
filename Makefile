all:
	mkdir -p build
	gcc -std=c99 -Wall -Werror -Wextra -Wcast-align -Wno-unused-parameter -pedantic src/parse.c src/test.c src/write.c -Iinclude -o build/smf
	gcc -std=c99 -Wall -Werror -Wextra -Wcast-align -Wno-unused-parameter -pedantic src/parse.c src/test_thru.c src/write.c -Iinclude -o build/smf_thru

clean:
	rm -rf build
