SRC := throttle.c

BIN := test_throttle

$(BIN):$(SRC)
	arm-linux-gnueabihf-gcc -g $(SRC) -o $(BIN) -lpthread

clean:
	rm -rf $(BIN)
