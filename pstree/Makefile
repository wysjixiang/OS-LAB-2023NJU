
SRC := pstree
OUT := pstree-native

mainargs ?=

.PHONY: make run clean

DEFAULT_GOAL: make

make:
	gcc -o $(OUT) $(SRC).c

run: make
	./$(OUT) $(mainargs)

clean:
	rm $(OUT)