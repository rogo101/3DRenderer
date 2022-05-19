build:
	gcc -Isrc/include -Lsrc/lib -Wall -std=c99 ./src/*.c -o renderer -lmingw32 -lSDL2main -lSDL2 -lm

run:
	./renderer

clean:
	rm renderer