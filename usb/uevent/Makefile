TARGET := UEvent
SRC := uevent.c main.c

$(TARGET):$(SRC)
	arm-linux-gnueabihf-gcc -g $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET)
