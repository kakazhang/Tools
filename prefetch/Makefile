CROSS_COMPILE 	?= arm-linux-gnueabihf-
TARGET		  	?= prefetch

CC 				:= $(CROSS_COMPILE)gcc

SRCDIRS			:= 	$(shell pwd)

CFILES		:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))

$(TARGET):$(CFILES)
	$(CC) -g $(CFILES) -o $(TARGET) -DREADAHEAD

clean:
	rm -rf $(TARGET)
