CC=gcc
# CC=${CROSS_COMPILE}gcc
CFLAGS  += -g -Wall

TARGET = aesdsocket
all: ${TARGET}

${TARGET}: ${TARGET}.c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm ${TARGET}
	rm -rf /var/tmp/aesdsocketdata