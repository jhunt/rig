CFLAGS += -Wall -Wpedantic

BINS :=
BINS += every
BINS += init
BINS += svcs

all: $(BINS)
stripped: $(BINS)
	strip -s $(BINS)

clean:
	rm -f *.o
	rm -f $(BINS)
