CC=gcc
CFLAGS=-Wall
LIB=-lpthread
TARGET=parallel_genfile
default:$(TARGET)
SOURCE=parallel_genfile.c

all : $(TARGET)

%o:%c 
	$(CC)  $(CFLAGS) -o $@ -c $(LIB) $< 

$(TARGET) : parallel_genfile.o
	$(CC) -o $@  $^ $(LIB)

.PHONY: all clean
clean:
	rm -rf *.o
	rm -rf $(TARGET)
