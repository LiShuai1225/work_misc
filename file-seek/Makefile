CC = gcc
TARGET=file_seek
src = $(wildcard *.c)
obj = $(src:.c=.o)

$(TARGET): $(obj)
	    $(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(TARGET)
