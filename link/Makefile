CROSS_COMPILE 	?= arm-linux-gnueabihf-
#CROSS_COMPILE 	?=
TARGET		  	?= LINK

CC 			:= $(CROSS_COMPILE)g++

INCDIRS 		:= 	inc/communication\
					inc/poll\
					inc/common \
					inc/link

SRCDIRS			:= 	src\
					src/communication\
					src/poll\
					src/link

INCLUDE			:= $(patsubst %, -I %, $(INCDIRS))

CPPFILES		:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))

CPPFLAGS        := -Wno-write-strings -lpthread

$(TARGET):$(CPPFILES)
	$(CC) -g $(CPPFILES) -o $(TARGET) $(INCLUDE) $(CPPFLAGS)

clean:
	rm -rf $(TARGET)
