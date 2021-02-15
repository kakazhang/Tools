SRC := latencytop.c

BIN := latencytop

CFLAGS := -Wall -Werror -Wno-unused-parameter

$(BIN):$(SRC)
	arm-linux-gnueabihf-gcc -g $(SRC) -o $(BIN) $(CFLAGS)

clean:
	rm -rf $(BIN)
