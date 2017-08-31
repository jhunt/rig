CFLAGS += -Wall -Wpedantic

all: every

stripped: every
	strip -s every

clean:
	rm -f *.o
	rm -f every

every: every.o
