CFLAGS += -Wall -Wpedantic

BINS :=
BINS += every
BINS += init
BINS += logto
BINS += supervise

all: $(BINS)
stripped: $(BINS)
	strip -s $(BINS)

clean:
	rm -f *.o
	rm -f $(BINS)
