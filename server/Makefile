CROSS_COMPILE=aarch64-none-linux-gnu-
CC=${CROSS_COMPILE}gcc
# CC=gcc
CFLAGS  += -g -Wall

TARGET = aesdsocket
all: ${TARGET}

${TARGET}: ${TARGET}.c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm ${TARGET}
	rm -rf /var/tmp/aesdsocketdata