CFLAGS += -Wall -Wpedantic

all: every init

stripped: every init
	strip -s every init

clean:
	rm -f *.o
	rm -f every

every: every.o
