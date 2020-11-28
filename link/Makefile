CROSS_COMPILE 	?= arm-linux-gnueabihf-
TARGET		  	?= LINK

CC 			:= $(CROSS_COMPILE)g++

INCDIRS 		:= 	inc/communication/\
					inc/tcpclient\
					inc/tcpserver\

SRCDIRS			:= 	src/\
					src/tcpclient\
					src/tcpserver\

INCLUDE			:= $(patsubst %, -I %, $(INCDIRS))

CPPFILES		:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))

CPPFLAGS        := -lpthread

$(TARGET):$(CPPFILES)
	$(CC) -g $(CPPFILES) -o $(TARGET) $(INCLUDE) $(CPPFLAGS)

clean:
	rm -rf $(TARGET)
