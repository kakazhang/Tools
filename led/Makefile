SRC := led_gpio.c

BIN := GLed

$(BIN):$(SRC)
	arm-linux-gnueabihf-gcc -g $(SRC) -o $(BIN)

clean:
	rm -rf $(BIN)
