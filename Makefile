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

every: every.o rig.o
init: init.o rig.o
logto: logto.o rig.o
supervise: supervise.o rig.o
