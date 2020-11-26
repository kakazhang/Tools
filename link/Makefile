CROSS_COMPILE 	?= arm-linux-gnueabihf-
TARGET		  	?= LINK

CC 			:= $(CROSS_COMPILE)g++

INCDIRS 		:= 	inc/link/\
				inc/tcpclient\
				inc/tcpserver\

SRCDIRS			:= 	src/\
				src/tcpserver\

INCLUDE			:= $(patsubst %, -I %, $(INCDIRS))

CPPFILES		:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))

$(TARGET):$(CPPFILES)
	$(CC) -g $(CPPFILES) -o $(TARGET) $(INCLUDE)

clean:
	rm -rf $(TARGET)
