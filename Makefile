TARGET=decomposition_main
#TARGET=polynomial_info_main

CC=gcc
OPT=-g -Wno-format
LIB= -lm -lpthread


SRC=$(TARGET).c
HED=$(shell ls *.h)

all: $(TARGET)
$(TARGET): $(SRC) $(HED)
	$(CC) $(OPT) -o $(TARGET)  $(SRC) $(LIB) 

clean:
	rm -f $(TARGET)
