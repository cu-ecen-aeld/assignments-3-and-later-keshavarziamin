# CC=gcc
CC=${CROSS_COMPILE}gcc
CFLAGS  = -g -Wall

TARGET = writer
all: ${TARGET}

${TARGET}: ${TARGET}.c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm ${TARGET}